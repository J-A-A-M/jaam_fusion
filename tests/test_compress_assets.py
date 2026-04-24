"""
Tests for tools/compress_assets.py

Tests cover the functions added/changed in this PR:
- compress_file(): gzip compression and hash calculation
- bytes_to_c_array(): C++ hex array formatting
- generate_header(): full header file generation including new login.html asset
"""

import gzip
import hashlib
import io
import os
import re
import sys
import tempfile
import textwrap
import unittest

# Allow importing the tools module from a standalone context
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS_DIR = os.path.join(REPO_ROOT, "tools")
sys.path.insert(0, TOOLS_DIR)

# Import the functions under test without triggering generate_header() at import time.
# compress_assets.py calls generate_header() at module level, so we import its
# functions by exec-ing the source with the call guarded.
import importlib
import importlib.util
import types


def _load_compress_assets_functions():
    """
    Load compress_file, bytes_to_c_array and generate_header from
    compress_assets.py without executing the top-level generate_header() call
    or triggering the PlatformIO Import() lookup.
    """
    src_path = os.path.join(TOOLS_DIR, "compress_assets.py")
    with open(src_path, "r") as fh:
        source = fh.read()

    # Replace the PlatformIO try/except block with a simple standalone assignment.
    # The block looks like:
    #   try:
    #       Import("env")
    #       ...
    #   except (NameError, KeyError):
    #       PROJECT_DIR = os.path.dirname(...)
    #       print(...)
    try_except_pattern = re.compile(
        r"# Check if running.*?(?=\nWEB_DIR)",
        re.DOTALL,
    )
    replacement = (
        "PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))\n"
    )
    source = try_except_pattern.sub(replacement, source)

    # Remove the bare top-level generate_header() call (last statement in file).
    source = re.sub(r"^generate_header\(\)\s*$", "", source, flags=re.MULTILINE)

    module = types.ModuleType("compress_assets")
    module.__file__ = src_path
    exec(compile(source, src_path, "exec"), module.__dict__)  # noqa: S102
    return module


_mod = _load_compress_assets_functions()
compress_file = _mod.compress_file
bytes_to_c_array = _mod.bytes_to_c_array
generate_header = _mod.generate_header
ASSETS = _mod.ASSETS


# ---------------------------------------------------------------------------
# compress_file()
# ---------------------------------------------------------------------------

class TestCompressFile(unittest.TestCase):
    """Tests for compress_file() introduced/used for all assets including login.html."""

    def _write_tmp(self, content: bytes) -> str:
        fd, path = tempfile.mkstemp()
        try:
            with os.fdopen(fd, "wb") as fh:
                fh.write(content)
        except Exception:
            os.close(fd)
            raise
        return path

    def test_returns_valid_gzip_data(self):
        """compress_file() must return data that can be decompressed back to the original."""
        original = b"Hello JAAM firmware web auth!"
        path = self._write_tmp(original)
        try:
            compressed, _, _, _ = compress_file(path)
            assert gzip.decompress(compressed) == original
        finally:
            os.unlink(path)

    def test_hash_is_16_hex_chars(self):
        """Hash returned must be exactly 16 lowercase hex characters (truncated SHA-256)."""
        path = self._write_tmp(b"test content")
        try:
            _, file_hash, _, _ = compress_file(path)
            assert len(file_hash) == 16, f"Expected 16 chars, got {len(file_hash)}"
            assert all(c in "0123456789abcdef" for c in file_hash), (
                f"Hash contains non-hex chars: {file_hash}"
            )
        finally:
            os.unlink(path)

    def test_hash_matches_sha256_of_compressed(self):
        """Hash must equal the first 16 chars of sha256(compressed_bytes)."""
        content = b"deterministic content for hashing"
        path = self._write_tmp(content)
        try:
            compressed, file_hash, _, _ = compress_file(path)
            expected = hashlib.sha256(compressed).hexdigest()[:16]
            assert file_hash == expected
        finally:
            os.unlink(path)

    def test_original_size_matches_input(self):
        """Third return value must equal the byte length of the original file."""
        content = b"X" * 200
        path = self._write_tmp(content)
        try:
            _, _, orig_size, _ = compress_file(path)
            assert orig_size == 200
        finally:
            os.unlink(path)

    def test_compressed_size_matches_data_length(self):
        """Fourth return value must equal len(compressed bytes)."""
        content = b"A" * 500
        path = self._write_tmp(content)
        try:
            compressed, _, _, comp_size = compress_file(path)
            assert comp_size == len(compressed)
        finally:
            os.unlink(path)

    def test_deterministic_output_mtime_zero(self):
        """Same content must always produce identical compressed bytes (mtime=0)."""
        content = b"deterministic gzip output test"
        path = self._write_tmp(content)
        try:
            compressed_a, hash_a, _, _ = compress_file(path)
            compressed_b, hash_b, _, _ = compress_file(path)
            assert compressed_a == compressed_b
            assert hash_a == hash_b
        finally:
            os.unlink(path)

    def test_compression_level_9(self):
        """Output must use gzip compression level 9 (verified via header byte)."""
        # gzip format: bytes[8] is XFL field; 0x02 = level 9 (max compression)
        content = b"compress me at max level " * 100
        path = self._write_tmp(content)
        try:
            compressed, _, _, _ = compress_file(path)
            # Byte index 8 is the XFL (extra flags) field
            assert compressed[8] == 0x02, (
                f"Expected XFL=0x02 (max compression), got 0x{compressed[8]:02x}"
            )
        finally:
            os.unlink(path)

    def test_empty_file(self):
        """compress_file() must handle an empty file without error."""
        path = self._write_tmp(b"")
        try:
            compressed, file_hash, orig_size, comp_size = compress_file(path)
            assert orig_size == 0
            assert comp_size == len(compressed)
            assert len(file_hash) == 16
        finally:
            os.unlink(path)

    def test_binary_content(self):
        """compress_file() must handle arbitrary binary content."""
        content = bytes(range(256)) * 10
        path = self._write_tmp(content)
        try:
            compressed, _, orig_size, _ = compress_file(path)
            assert gzip.decompress(compressed) == content
            assert orig_size == len(content)
        finally:
            os.unlink(path)


# ---------------------------------------------------------------------------
# bytes_to_c_array()
# ---------------------------------------------------------------------------

class TestBytesToCArray(unittest.TestCase):
    """Tests for bytes_to_c_array() used to format all web assets including login.html."""

    def test_output_starts_with_correct_declaration(self):
        """Output must start with 'const uint8_t <name>[] PROGMEM = {'."""
        result = bytes_to_c_array(b"\x00\x01", "my_asset_gz")
        assert result.startswith("const uint8_t my_asset_gz[] PROGMEM = {")

    def test_output_ends_with_closing_brace_semicolon(self):
        """Output must end with '};'."""
        result = bytes_to_c_array(b"\xff", "test_gz")
        assert result.endswith("};")

    def test_single_byte_hex_format(self):
        """Single byte must be formatted as '0xNN'."""
        result = bytes_to_c_array(b"\xAB", "var_gz")
        assert "0xab" in result

    def test_multiple_bytes_comma_separated(self):
        """Multiple bytes must be comma-separated."""
        result = bytes_to_c_array(b"\x01\x02\x03", "var_gz")
        assert "0x01" in result
        assert "0x02" in result
        assert "0x03" in result

    def test_16_bytes_per_line(self):
        """Each data line must contain at most 16 hex values."""
        data = bytes(range(32))  # 32 bytes -> 2 full lines
        result = bytes_to_c_array(data, "var_gz")
        # Extract content lines (between { and })
        lines = result.split("\n")
        data_lines = [l.strip() for l in lines if l.strip() and not l.strip().startswith("const")]
        data_lines = [l.rstrip(",") for l in data_lines]
        for line in data_lines:
            count = len([x for x in line.split(",") if x.strip()])
            assert count <= 16, f"Line has {count} bytes, expected <= 16: {line}"

    def test_exactly_16_bytes_is_single_line(self):
        """Exactly 16 bytes must produce exactly one data line."""
        data = bytes(range(16))
        result = bytes_to_c_array(data, "var_gz")
        lines = result.split("\n")
        data_lines = [l for l in lines if "0x" in l]
        assert len(data_lines) == 1

    def test_17_bytes_spans_two_lines(self):
        """17 bytes must produce two data lines (16 + 1)."""
        data = bytes(range(17))
        result = bytes_to_c_array(data, "var_gz")
        lines = result.split("\n")
        data_lines = [l for l in lines if "0x" in l]
        assert len(data_lines) == 2

    def test_variable_name_embedded(self):
        """The variable name must appear in the declaration."""
        result = bytes_to_c_array(b"\x00", "login_html_gz")
        assert "login_html_gz" in result

    def test_zero_byte_formatting(self):
        """Zero byte must be formatted as '0x00'."""
        result = bytes_to_c_array(b"\x00", "var_gz")
        assert "0x00" in result

    def test_max_byte_formatting(self):
        """Max byte (0xFF) must be formatted as '0xff'."""
        result = bytes_to_c_array(b"\xFF", "var_gz")
        assert "0xff" in result

    def test_all_bytes_present(self):
        """All 256 possible byte values must appear correctly when provided."""
        data = bytes(range(256))
        result = bytes_to_c_array(data, "var_gz")
        for i in range(256):
            assert f"0x{i:02x}" in result


# ---------------------------------------------------------------------------
# ASSETS dictionary
# ---------------------------------------------------------------------------

class TestAssetsDefinition(unittest.TestCase):
    """Tests for the ASSETS dict to verify login.html was added in this PR."""

    def test_login_html_is_in_assets(self):
        """login.html must be listed as a compressible asset."""
        assert "login_html" in ASSETS, "login_html not found in ASSETS"

    def test_login_html_path_ends_correctly(self):
        """login.html asset path must end with 'login.html'."""
        path = ASSETS["login_html"]
        assert path.endswith("login.html"), f"Unexpected path: {path}"

    def test_login_html_file_exists(self):
        """The actual login.html file must exist on disk."""
        path = ASSETS["login_html"]
        assert os.path.exists(path), f"login.html not found at: {path}"

    def test_all_asset_files_exist(self):
        """All declared assets must exist on disk (regression guard)."""
        missing = [
            name for name, path in ASSETS.items() if not os.path.exists(path)
        ]
        assert not missing, f"Missing asset files: {missing}"

    def test_assets_count(self):
        """ASSETS must include at least 12 entries (all original + login.html)."""
        assert len(ASSETS) >= 12, f"Expected >= 12 assets, got {len(ASSETS)}"


# ---------------------------------------------------------------------------
# generate_header() integration
# ---------------------------------------------------------------------------

class TestGenerateHeader(unittest.TestCase):
    """Integration tests for generate_header() that exercises the full pipeline."""

    def _run_with_output_dir(self, tmp_dir):
        """
        Monkey-patch OUTPUT_FILE to write into tmp_dir, then run generate_header().
        Returns the path to the generated file.
        """
        output_path = os.path.join(tmp_dir, "web_assets.h")
        original_output = _mod.OUTPUT_FILE
        _mod.OUTPUT_FILE = output_path
        try:
            generate_header()
        finally:
            _mod.OUTPUT_FILE = original_output
        return output_path

    def test_header_guard_present(self):
        """Generated header must include include guard JAAM_WEB_ASSETS_H."""
        with tempfile.TemporaryDirectory() as tmp:
            output = self._run_with_output_dir(tmp)
            content = open(output).read()
        assert "#ifndef JAAM_WEB_ASSETS_H" in content
        assert "#define JAAM_WEB_ASSETS_H" in content
        assert "#endif" in content

    def test_login_html_symbols_generated(self):
        """Generated header must contain login_html_gz, login_html_gz_len, login_html_hash."""
        with tempfile.TemporaryDirectory() as tmp:
            output = self._run_with_output_dir(tmp)
            content = open(output).read()
        assert "login_html_gz[]" in content, "login_html_gz array missing"
        assert "login_html_gz_len" in content, "login_html_gz_len missing"
        assert "login_html_hash[]" in content, "login_html_hash missing"

    def test_login_html_hash_is_hex_string(self):
        """login_html_hash value must be a 16-char hex string."""
        with tempfile.TemporaryDirectory() as tmp:
            output = self._run_with_output_dir(tmp)
            content = open(output).read()
        import re
        match = re.search(r'const char login_html_hash\[\] = "([0-9a-f]{16})"', content)
        assert match is not None, "login_html_hash not found or wrong format"

    def test_all_assets_generate_symbols(self):
        """Every asset must generate _gz[], _gz_len, and _hash[] symbols."""
        with tempfile.TemporaryDirectory() as tmp:
            output = self._run_with_output_dir(tmp)
            content = open(output).read()
        for name in ASSETS:
            assert f"{name}_gz[]" in content, f"Missing {name}_gz[]"
            assert f"{name}_gz_len" in content, f"Missing {name}_gz_len"
            assert f"{name}_hash[]" in content, f"Missing {name}_hash[]"

    def test_progmem_keyword_present(self):
        """All arrays must use PROGMEM keyword (required for ESP32 flash storage)."""
        with tempfile.TemporaryDirectory() as tmp:
            output = self._run_with_output_dir(tmp)
            content = open(output).read()
        assert "PROGMEM" in content

    def test_header_is_valid_utf8(self):
        """Generated header file must be valid UTF-8."""
        with tempfile.TemporaryDirectory() as tmp:
            output = self._run_with_output_dir(tmp)
            with open(output, encoding="utf-8") as fh:
                fh.read()  # raises UnicodeDecodeError if not UTF-8

    def test_missing_asset_raises_file_not_found(self):
        """generate_header() must raise FileNotFoundError when an asset file is missing."""
        with tempfile.TemporaryDirectory() as tmp:
            # Temporarily add a nonexistent asset
            _mod.ASSETS["__nonexistent__"] = os.path.join(tmp, "does_not_exist.txt")
            try:
                with self.assertRaises(FileNotFoundError):
                    self._run_with_output_dir(tmp)
            finally:
                del _mod.ASSETS["__nonexistent__"]

    def test_login_html_compressed_data_decompressible(self):
        """The login_html_gz bytes embedded in the header must decompress to valid HTML."""
        with tempfile.TemporaryDirectory() as tmp:
            output = self._run_with_output_dir(tmp)
            content = open(output).read()

        # Extract the hex bytes from the login_html_gz array
        import re
        # Match the array body between { and };
        pattern = r"const uint8_t login_html_gz\[\] PROGMEM = \{([\s\S]*?)\};"
        match = re.search(pattern, content)
        assert match is not None, "Could not find login_html_gz array"

        hex_str = match.group(1)
        byte_values = re.findall(r"0x([0-9a-f]{2})", hex_str)
        raw = bytes(int(h, 16) for h in byte_values)
        decompressed = gzip.decompress(raw)
        assert b"<!DOCTYPE html>" in decompressed or b"<html" in decompressed, (
            "Decompressed login_html_gz does not look like HTML"
        )

    def test_auto_generated_comment_present(self):
        """Header must contain the auto-generated warning comment."""
        with tempfile.TemporaryDirectory() as tmp:
            output = self._run_with_output_dir(tmp)
            content = open(output).read()
        assert "DO NOT EDIT MANUALLY" in content

    def test_compression_ratio_comment_in_header(self):
        """Each asset comment line must include '→' size reduction info."""
        with tempfile.TemporaryDirectory() as tmp:
            output = self._run_with_output_dir(tmp)
            content = open(output).read()
        assert "→" in content, "Size reduction comment with '→' not found"
        assert "reduction" in content, "Compression ratio comment missing"


if __name__ == "__main__":
    unittest.main()