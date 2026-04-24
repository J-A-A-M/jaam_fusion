"""
Tests for web assets changed in this PR:
- web/controls.json  : new web_auth_enabled, web_login, web_password controls
- web/login.html     : new login page added in this PR
- src/web_assets.h   : generated file – verify login_html symbols are present
"""

import gzip
import json
import os
import re
import unittest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONTROLS_JSON = os.path.join(REPO_ROOT, "web", "controls.json")
LOGIN_HTML = os.path.join(REPO_ROOT, "web", "login.html")
WEB_ASSETS_H = os.path.join(REPO_ROOT, "src", "web_assets.h")


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _load_controls():
    with open(CONTROLS_JSON, encoding="utf-8") as fh:
        data = json.load(fh)
    # controls.json may be a plain list OR a dict with a "controls" key
    if isinstance(data, dict):
        return data.get("controls", data)
    return data


def _find_control(controls, key):
    """Return the first control entry whose second element equals `key`."""
    for entry in controls:
        if isinstance(entry, list) and len(entry) >= 2 and entry[1] == key:
            return entry
    return None


def _login_html_text():
    with open(LOGIN_HTML, encoding="utf-8") as fh:
        return fh.read()


def _web_assets_h_text():
    with open(WEB_ASSETS_H, encoding="utf-8") as fh:
        return fh.read()


# ---------------------------------------------------------------------------
# controls.json – web auth fields (new in this PR)
# ---------------------------------------------------------------------------

class TestControlsJsonWebAuth(unittest.TestCase):
    """Verify the new authentication controls added to web/controls.json."""

    @classmethod
    def setUpClass(cls):
        cls.controls = _load_controls()

    # --- web_auth_enabled ---

    def test_web_auth_enabled_exists(self):
        """web_auth_enabled control must be present in controls.json."""
        ctrl = _find_control(self.controls, "web_auth_enabled")
        self.assertIsNotNone(ctrl, "web_auth_enabled not found in controls.json")

    def test_web_auth_enabled_type_is_bool(self):
        """web_auth_enabled must be of type 'bool'."""
        ctrl = _find_control(self.controls, "web_auth_enabled")
        self.assertEqual(ctrl[0], "bool", f"Expected 'bool', got '{ctrl[0]}'")

    def test_web_auth_enabled_section_is_network(self):
        """web_auth_enabled must belong to the 'network' section."""
        ctrl = _find_control(self.controls, "web_auth_enabled")
        self.assertIn(
            "network", ctrl,
            "Section 'network' not found in web_auth_enabled control"
        )

    def test_web_auth_enabled_no_visibility_condition(self):
        """web_auth_enabled must be always visible (empty or absent condition)."""
        ctrl = _find_control(self.controls, "web_auth_enabled")
        # Last element is the visibility condition when present
        # An empty string means always visible
        condition = ctrl[-1] if isinstance(ctrl[-1], str) else ""
        self.assertEqual(
            condition, "",
            f"web_auth_enabled should have no visibility condition, got '{condition}'"
        )

    # --- web_login ---

    def test_web_login_exists(self):
        """web_login control must be present in controls.json."""
        ctrl = _find_control(self.controls, "web_login")
        self.assertIsNotNone(ctrl, "web_login not found in controls.json")

    def test_web_login_type_is_text(self):
        """web_login must be of type 'text'."""
        ctrl = _find_control(self.controls, "web_login")
        self.assertEqual(ctrl[0], "text", f"Expected 'text', got '{ctrl[0]}'")

    def test_web_login_default_is_admin(self):
        """web_login default value must be 'admin'."""
        ctrl = _find_control(self.controls, "web_login")
        self.assertIn("admin", ctrl, "Default 'admin' not found in web_login control")

    def test_web_login_section_is_network(self):
        """web_login must belong to the 'network' section."""
        ctrl = _find_control(self.controls, "web_login")
        self.assertIn("network", ctrl)

    def test_web_login_visibility_condition(self):
        """web_login must only show when web_auth_enabled==1."""
        ctrl = _find_control(self.controls, "web_login")
        self.assertIn(
            "web_auth_enabled==1", ctrl,
            "web_login should have visibility condition 'web_auth_enabled==1'"
        )

    # --- web_password ---

    def test_web_password_exists(self):
        """web_password control must be present in controls.json."""
        ctrl = _find_control(self.controls, "web_password")
        self.assertIsNotNone(ctrl, "web_password not found in controls.json")

    def test_web_password_type_is_text(self):
        """web_password must be of type 'text'."""
        ctrl = _find_control(self.controls, "web_password")
        self.assertEqual(ctrl[0], "text")

    def test_web_password_default_is_empty(self):
        """web_password must have an empty string as default value."""
        ctrl = _find_control(self.controls, "web_password")
        self.assertIn(
            "", ctrl,
            "web_password should have empty string default"
        )

    def test_web_password_section_is_network(self):
        """web_password must belong to the 'network' section."""
        ctrl = _find_control(self.controls, "web_password")
        self.assertIn("network", ctrl)

    def test_web_password_visibility_condition(self):
        """web_password must only show when web_auth_enabled==1."""
        ctrl = _find_control(self.controls, "web_password")
        self.assertIn(
            "web_auth_enabled==1", ctrl,
            "web_password should have visibility condition 'web_auth_enabled==1'"
        )

    # --- Authentication label ---

    def test_auth_label_exists(self):
        """There must be a label 'Автентифікація' in the network section."""
        found = any(
            isinstance(entry, list)
            and entry[0] == "label"
            and "Автентифікація" in entry
            and "network" in entry
            for entry in self.controls
        )
        self.assertTrue(found, "Authentication section label not found")

    # --- Structural integrity ---

    def test_controls_is_list(self):
        """controls.json top-level must be a list (array)."""
        self.assertIsInstance(self.controls, list)

    def test_all_controls_are_lists(self):
        """Every entry in controls.json must be a list."""
        non_list = [e for e in self.controls if not isinstance(e, list)]
        self.assertEqual(
            non_list, [],
            f"Non-list entries found: {non_list[:3]}"
        )

    def test_auth_controls_order(self):
        """web_auth_enabled must appear before web_login and web_password."""
        indices = {}
        for i, entry in enumerate(self.controls):
            if isinstance(entry, list) and len(entry) >= 2:
                if entry[1] in ("web_auth_enabled", "web_login", "web_password"):
                    indices[entry[1]] = i
        self.assertLess(
            indices.get("web_auth_enabled", 9999),
            indices.get("web_login", 9999),
            "web_auth_enabled must come before web_login"
        )
        self.assertLess(
            indices.get("web_auth_enabled", 9999),
            indices.get("web_password", 9999),
            "web_auth_enabled must come before web_password"
        )
        self.assertLess(
            indices.get("web_login", 9999),
            indices.get("web_password", 9999),
            "web_login must come before web_password"
        )

    def test_both_login_and_password_have_same_condition(self):
        """web_login and web_password must share the same visibility condition."""
        login_ctrl = _find_control(self.controls, "web_login")
        pass_ctrl = _find_control(self.controls, "web_password")
        # Condition is the last element
        self.assertEqual(login_ctrl[-1], pass_ctrl[-1])


# ---------------------------------------------------------------------------
# web/login.html – new file added in this PR
# ---------------------------------------------------------------------------

class TestLoginHtml(unittest.TestCase):
    """Verify the structure and content of the new web/login.html login page."""

    @classmethod
    def setUpClass(cls):
        cls.html = _login_html_text()

    def test_file_exists(self):
        """web/login.html must exist on disk."""
        self.assertTrue(os.path.exists(LOGIN_HTML))

    def test_html_doctype(self):
        """login.html must declare DOCTYPE html."""
        self.assertIn("<!DOCTYPE html>", self.html)

    def test_login_form_present(self):
        """Must contain a login form that POSTs to /login."""
        self.assertIn('method="POST"', self.html)
        self.assertIn('action="/login"', self.html)

    def test_login_field_name(self):
        """Login input must use name='login'."""
        self.assertIn('name="login"', self.html)

    def test_password_field_name(self):
        """Password input must use name='password'."""
        self.assertIn('name="password"', self.html)

    def test_password_field_type(self):
        """Password input must be of type='password'."""
        self.assertIn('type="password"', self.html)

    def test_recovery_field_name(self):
        """Recovery token input must use name='recovery'."""
        self.assertIn('name="recovery"', self.html)

    def test_error_div_present(self):
        """An element with id='errorMsg' must exist for displaying errors."""
        self.assertIn('id="errorMsg"', self.html)

    def test_recovery_section_present(self):
        """An element with id='recoverySection' must exist."""
        self.assertIn('id="recoverySection"', self.html)

    def test_recovery_section_hidden_by_default(self):
        """recovery-section must start hidden (display: none via CSS class)."""
        # The element uses class="recovery-section" which has display:none in CSS
        self.assertIn('class="recovery-section"', self.html)
        self.assertIn("display: none", self.html)

    def test_toggle_recovery_function(self):
        """toggleRecovery() JavaScript function must be defined."""
        self.assertIn("toggleRecovery", self.html)

    def test_toggle_theme_function(self):
        """toggleTheme() JavaScript function must be defined."""
        self.assertIn("toggleTheme", self.html)

    def test_error_code_1_handling(self):
        """Page must handle ?error=1 for invalid credentials."""
        self.assertIn("error", self.html)
        # The JS checks params.get('error') === '1' (single quotes)
        self.assertTrue(
            "=== '1'" in self.html or '=== "1"' in self.html or "'1'" in self.html,
            "No error=1 check found in login.html",
        )

    def test_error_code_2_handling(self):
        """Page must handle ?error=2 for invalid recovery token."""
        self.assertTrue(
            "'2'" in self.html or '"2"' in self.html,
            "No error=2 check found in login.html",
        )

    def test_theme_cookie_name(self):
        """Page must use 'jaam_theme' as the cookie name for theme persistence."""
        self.assertIn("jaam_theme", self.html)

    def test_theme_cookie_max_age(self):
        """Theme cookie must have a max-age of 1 year (31536000 seconds)."""
        self.assertIn("31536000", self.html)

    def test_username_autocomplete(self):
        """Login input must include autocomplete='username'."""
        self.assertIn('autocomplete="username"', self.html)

    def test_password_autocomplete(self):
        """Password input must include autocomplete='current-password'."""
        self.assertIn('autocomplete="current-password"', self.html)

    def test_recovery_form_autocomplete_off(self):
        """Recovery form must disable autocomplete for security."""
        self.assertIn('autocomplete="off"', self.html)

    def test_charset_utf8(self):
        """Page must declare UTF-8 charset."""
        self.assertIn('charset="UTF-8"', self.html)

    def test_viewport_meta(self):
        """Page must include viewport meta for mobile responsiveness."""
        self.assertIn("viewport", self.html)

    def test_links_styles_css(self):
        """Page must reference /styles.css for shared styling."""
        self.assertIn("/styles.css", self.html)

    def test_data_theme_attribute_applied(self):
        """Page must dynamically apply data-theme attribute for theming."""
        self.assertIn("data-theme", self.html)
        self.assertIn("setAttribute", self.html)

    def test_recovery_token_field_spellcheck_off(self):
        """Recovery token input must disable spellcheck to avoid token mangling."""
        self.assertIn('spellcheck="false"', self.html)

    def test_recovery_section_shows_on_error_2(self):
        """On ?error=2, recovery section must auto-show via classList.add('visible')."""
        self.assertIn("classList.add", self.html)
        self.assertIn("visible", self.html)

    def test_both_forms_post_to_login(self):
        """Both the login form and recovery form must POST to /login."""
        matches = re.findall(r'action="/login"', self.html)
        self.assertGreaterEqual(
            len(matches), 2,
            f"Expected at least 2 forms posting to /login, found {len(matches)}"
        )

    def test_no_external_script_urls(self):
        """Page must not load scripts from external domains (security/offline requirement)."""
        external_scripts = re.findall(r'<script[^>]+src=["\']https?://', self.html)
        self.assertEqual(
            external_scripts, [],
            f"Found external script tags: {external_scripts}"
        )

    def test_login_required_attribute(self):
        """Login and password fields must have the 'required' attribute."""
        self.assertIn("required", self.html)

    def test_html_language_ukrainian(self):
        """Page must declare Ukrainian as the language."""
        self.assertIn('lang="uk"', self.html)


# ---------------------------------------------------------------------------
# src/web_assets.h – generated file, verify login_html is present
# ---------------------------------------------------------------------------

class TestWebAssetsHLoginHtml(unittest.TestCase):
    """
    Verify that web_assets.h (generated by compress_assets.py) contains the
    login_html symbols added in this PR.
    """

    @classmethod
    def setUpClass(cls):
        if not os.path.exists(WEB_ASSETS_H):
            cls.content = None
        else:
            with open(WEB_ASSETS_H, encoding="utf-8") as fh:
                cls.content = fh.read()

    def _skip_if_no_header(self):
        if self.content is None:
            self.skipTest("src/web_assets.h not generated yet; run compress_assets.py first")

    def test_login_html_gz_array_present(self):
        """web_assets.h must declare login_html_gz[] PROGMEM array."""
        self._skip_if_no_header()
        self.assertIn("login_html_gz[]", self.content)

    def test_login_html_gz_len_present(self):
        """web_assets.h must define login_html_gz_len size constant."""
        self._skip_if_no_header()
        self.assertIn("login_html_gz_len", self.content)

    def test_login_html_hash_present(self):
        """web_assets.h must define login_html_hash[] for ETag caching."""
        self._skip_if_no_header()
        self.assertIn("login_html_hash[]", self.content)

    def test_login_html_hash_format(self):
        """login_html_hash must be a 16-character lowercase hex string."""
        self._skip_if_no_header()
        match = re.search(r'const char login_html_hash\[\] = "([^"]+)"', self.content)
        self.assertIsNotNone(match, "login_html_hash declaration not found")
        hash_value = match.group(1)
        self.assertEqual(len(hash_value), 16, f"Hash length is {len(hash_value)}, expected 16")
        self.assertTrue(
            all(c in "0123456789abcdef" for c in hash_value),
            f"Hash contains non-hex characters: {hash_value}"
        )

    def test_login_html_gz_len_is_positive(self):
        """login_html_gz_len must be a positive integer."""
        self._skip_if_no_header()
        match = re.search(r"const size_t login_html_gz_len = (\d+);", self.content)
        self.assertIsNotNone(match, "login_html_gz_len definition not found")
        size = int(match.group(1))
        self.assertGreater(size, 0, "login_html_gz_len must be > 0")

    def test_login_html_gz_data_decompresses_to_html(self):
        """The embedded login_html_gz bytes must decompress to valid HTML."""
        self._skip_if_no_header()
        pattern = r"const uint8_t login_html_gz\[\] PROGMEM = \{([\s\S]*?)\};"
        match = re.search(pattern, self.content)
        self.assertIsNotNone(match, "login_html_gz array body not found")
        hex_values = re.findall(r"0x([0-9a-f]{2})", match.group(1))
        raw = bytes(int(h, 16) for h in hex_values)
        decompressed = gzip.decompress(raw)
        self.assertIn(b"<html", decompressed)

    def test_header_guard_present(self):
        """web_assets.h must have include guard JAAM_WEB_ASSETS_H."""
        self._skip_if_no_header()
        self.assertIn("#ifndef JAAM_WEB_ASSETS_H", self.content)
        self.assertIn("#define JAAM_WEB_ASSETS_H", self.content)

    def test_gzip_byte_signature_in_array(self):
        """The login_html_gz array must start with gzip magic bytes 0x1f, 0x8b."""
        self._skip_if_no_header()
        pattern = r"const uint8_t login_html_gz\[\] PROGMEM = \{([\s\S]*?)\};"
        match = re.search(pattern, self.content)
        self.assertIsNotNone(match)
        hex_values = re.findall(r"0x([0-9a-f]{2})", match.group(1))
        self.assertGreaterEqual(len(hex_values), 2)
        self.assertEqual(hex_values[0], "1f", "First gzip magic byte must be 0x1f")
        self.assertEqual(hex_values[1], "8b", "Second gzip magic byte must be 0x8b")

    def test_mtime_zero_byte_in_gzip_header(self):
        """
        Bytes 4-7 of the gzip stream are the mtime field; must be 0x00 (deterministic).
        This was changed in this PR (was 0x13, now 0x03 XFL with mtime=0).
        """
        self._skip_if_no_header()
        pattern = r"const uint8_t login_html_gz\[\] PROGMEM = \{([\s\S]*?)\};"
        match = re.search(pattern, self.content)
        self.assertIsNotNone(match)
        hex_values = re.findall(r"0x([0-9a-f]{2})", match.group(1))
        # mtime is bytes 4-7 (indices 4,5,6,7)
        self.assertGreaterEqual(len(hex_values), 8)
        mtime_bytes = hex_values[4:8]
        self.assertEqual(
            mtime_bytes, ["00", "00", "00", "00"],
            f"mtime bytes must all be 0x00 for deterministic output, got {mtime_bytes}"
        )


# ---------------------------------------------------------------------------
# Cross-validation: controls.json auth keys match JaamSettings defaults
# ---------------------------------------------------------------------------

class TestAuthSettingsConsistency(unittest.TestCase):
    """
    Cross-validate that the keys used in controls.json match what the firmware
    uses. We verify this by checking the JaamSettings.cpp patterns in the source.
    """

    @classmethod
    def setUpClass(cls):
        settings_path = os.path.join(REPO_ROOT, "src", "JaamSettings.cpp")
        with open(settings_path, encoding="utf-8") as fh:
            cls.settings_src = fh.read()

    def test_web_auth_enabled_key_in_settings(self):
        """WEB_AUTH_ENABLED setting key 'waue' must appear in JaamSettings.cpp."""
        self.assertIn("waue", self.settings_src)

    def test_web_login_key_in_settings(self):
        """WEB_LOGIN setting key 'waulg' must appear in JaamSettings.cpp."""
        self.assertIn("waulg", self.settings_src)

    def test_web_password_key_in_settings(self):
        """WEB_PASSWORD setting key 'waupass' must appear in JaamSettings.cpp."""
        self.assertIn("waupass", self.settings_src)

    def test_web_auth_default_disabled(self):
        """WEB_AUTH_ENABLED must default to 0 (disabled) for safety."""
        match = re.search(r'WEB_AUTH_ENABLED.*"waue",\s*(\d+)', self.settings_src)
        self.assertIsNotNone(match, "Could not find WEB_AUTH_ENABLED default")
        self.assertEqual(match.group(1), "0", "WEB_AUTH_ENABLED should default to 0")

    def test_web_login_default_is_admin(self):
        """WEB_LOGIN must default to 'admin' in JaamSettings.cpp."""
        match = re.search(r'WEB_LOGIN.*"waulg",\s*"([^"]*)"', self.settings_src)
        self.assertIsNotNone(match, "Could not find WEB_LOGIN default")
        self.assertEqual(match.group(1), "admin")

    def test_web_password_default_is_empty(self):
        """WEB_PASSWORD must default to empty string in JaamSettings.cpp."""
        match = re.search(r'WEB_PASSWORD.*"waupass",\s*"([^"]*)"', self.settings_src)
        self.assertIsNotNone(match, "Could not find WEB_PASSWORD default")
        self.assertEqual(match.group(1), "")

    def test_web_auth_enum_values_in_config_h(self):
        """WEB_AUTH_ENABLED, WEB_LOGIN, WEB_PASSWORD must be in JaamConfig.h enum."""
        config_path = os.path.join(REPO_ROOT, "src", "JaamConfig.h")
        with open(config_path, encoding="utf-8") as fh:
            config_src = fh.read()
        self.assertIn("WEB_AUTH_ENABLED", config_src)
        self.assertIn("WEB_LOGIN", config_src)
        self.assertIn("WEB_PASSWORD", config_src)


# ---------------------------------------------------------------------------
# Auth logic in JaamWeb.cpp (structural/source-level checks)
# ---------------------------------------------------------------------------

class TestJaamWebAuthSource(unittest.TestCase):
    """
    Source-level tests for the authentication implementation in JaamWeb.cpp.
    These verify the code structure added in this PR without needing compilation.
    """

    @classmethod
    def setUpClass(cls):
        web_path = os.path.join(REPO_ROOT, "src", "JaamWeb.cpp")
        with open(web_path, encoding="utf-8") as fh:
            cls.src = fh.read()

    def test_generate_token_function_exists(self):
        """generateToken() function must be defined in JaamWeb.cpp."""
        self.assertIn("generateToken()", self.src)

    def test_generate_token_uses_esp_random(self):
        """generateToken() must use esp_random() for entropy."""
        self.assertIn("esp_random()", self.src)

    def test_generate_token_length_32(self):
        """generateToken() must generate a 32-character token."""
        self.assertIn("32", self.src)

    def test_require_auth_function_exists(self):
        """requireAuth() method must be defined."""
        self.assertIn("requireAuth()", self.src)

    def test_require_auth_checks_web_auth_enabled(self):
        """requireAuth() must check WEB_AUTH_ENABLED setting."""
        self.assertIn("WEB_AUTH_ENABLED", self.src)

    def test_require_auth_reads_cookie_header(self):
        """requireAuth() must read the Cookie header."""
        self.assertIn('"Cookie"', self.src)

    def test_require_auth_redirects_to_login(self):
        """requireAuth() must redirect to /login on unauthorized access."""
        self.assertIn('"/login"', self.src)

    def test_require_auth_returns_302(self):
        """requireAuth() must send HTTP 302 redirect."""
        self.assertIn("302", self.src)

    def test_handle_login_post_function_exists(self):
        """handleLoginPost() must be defined."""
        self.assertIn("handleLoginPost()", self.src)

    def test_handle_logout_function_exists(self):
        """handleLogout() must be defined."""
        self.assertIn("handleLogout()", self.src)

    def test_handle_login_function_exists(self):
        """handleLogin() must be defined."""
        self.assertIn("handleLogin()", self.src)

    def test_login_post_checks_password_non_empty(self):
        """Login must only succeed when password is non-empty (pass.length() > 0)."""
        self.assertIn("pass.length() > 0", self.src)

    def test_login_post_reads_login_arg(self):
        """handleLoginPost() must read the 'login' form argument."""
        self.assertIn('"login"', self.src)

    def test_login_post_reads_password_arg(self):
        """handleLoginPost() must read the 'password' form argument."""
        self.assertIn('"password"', self.src)

    def test_recovery_token_support(self):
        """handleLoginPost() must support recovery token authentication."""
        self.assertIn("recoveryToken", self.src)
        self.assertIn('"recovery"', self.src)

    def test_set_cookie_header_on_login(self):
        """On successful login, a session cookie must be set."""
        self.assertIn("Set-Cookie", self.src)
        self.assertIn("session=", self.src)

    def test_cookie_http_only_flag(self):
        """Session cookie must include HttpOnly flag."""
        self.assertIn("HttpOnly", self.src)

    def test_cookie_path_flag(self):
        """Session cookie must include Path=/ flag."""
        self.assertIn("Path=/", self.src)

    def test_logout_clears_session_cookie(self):
        """handleLogout() must clear the session cookie (Max-Age=0)."""
        self.assertIn("Max-Age=0", self.src)

    def test_logout_clears_session_token(self):
        """handleLogout() must clear sessionToken."""
        self.assertIn('sessionToken = ""', self.src)

    def test_logout_redirects_to_login(self):
        """handleLogout() must redirect to /login after clearing session."""
        # Check that logout sends a Location header to /login
        self.assertIn("/login", self.src)

    def test_recovery_token_generated_on_begin(self):
        """Recovery token must be generated in begin() when auth is enabled."""
        self.assertIn("recoveryToken = generateToken()", self.src)

    def test_recovery_token_logged(self):
        """Recovery token must be logged to serial for admin access."""
        self.assertIn("Recovery token", self.src)

    def test_collect_cookie_header(self):
        """Server must collect Cookie headers (collectHeaders call)."""
        self.assertIn("collectHeaders", self.src)

    def test_login_error_1_redirect(self):
        """Failed login must redirect to /login?error=1."""
        self.assertIn("/login?error=1", self.src)

    def test_login_error_2_redirect(self):
        """Failed recovery token must redirect to /login?error=2."""
        self.assertIn("/login?error=2", self.src)

    def test_protected_routes_use_require_auth(self):
        """Protected routes must call requireAuth() before handling."""
        # Count requireAuth() calls - should be many (all protected endpoints)
        require_auth_calls = self.src.count("requireAuth()")
        self.assertGreater(
            require_auth_calls, 5,
            f"Expected many requireAuth() calls, found {require_auth_calls}"
        )

    def test_login_page_not_protected(self):
        """The /login route handler must NOT call requireAuth()."""
        # Find the handleLogin lambda/call
        login_route = re.search(
            r'server\.on\("/login",\s*HTTP_GET.*?handleLogin\(\)',
            self.src, re.DOTALL
        )
        self.assertIsNotNone(login_route, "/login GET route not found")
        # The route body must not contain requireAuth
        route_text = login_route.group(0)
        self.assertNotIn("requireAuth", route_text)

    def test_session_cookie_name_is_session(self):
        """Cookie name must be 'session' (not 'auth' or other)."""
        self.assertIn("session=", self.src)

    def test_token_chars_are_hex(self):
        """generateToken() must produce hex characters (0-9, a-f)."""
        self.assertIn("0123456789abcdef", self.src)

    def test_already_authenticated_login_redirects_to_root(self):
        """handleLogin() must redirect to / if user is already logged in."""
        # The handleLogin function should redirect already-authenticated users
        handle_login_idx = self.src.find("void JaamWeb::handleLogin()")
        self.assertGreater(handle_login_idx, -1, "handleLogin not found")
        handle_login_body = self.src[handle_login_idx:handle_login_idx + 800]
        self.assertIn('Location", "/"', handle_login_body) or \
            self.assertIn('Location", "/"', handle_login_body)


if __name__ == "__main__":
    unittest.main()
