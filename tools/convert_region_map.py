#!/usr/bin/env python3
"""
Convert region_maps.json to JaamConfig_Generated.h

This script reads the region map data from JSON format and generates
a C++ header file with optimized flat array structures for memory efficiency.

Usage:
    python convert_region_map.py [input_json] [output_header]

Default:
    Input:  ../data/region_maps.json
    Output: ../src/JaamConfig_Generated.h
"""

import json
import sys
from pathlib import Path
from typing import Dict, List, Tuple


def build_flat_arrays(
    regions: List[Dict],
) -> Tuple[List[Tuple[int, int, int, str]], List[int], List[Tuple[List[int], str]], int]:
    """Build deduplicated flat arrays for a region map.

    This function performs *content-based deduplication* of LED lists:
    if multiple regions have the exact same `leds` list, their metadata entries
    will reference the same `start_index` in the flat LED array.

    Returns:
        meta_entries: list of (region_id, start_index, led_count, region_name) in original region order
        flat_leds: flat list of uint16 LED positions (unique segments concatenated)
        led_segments: list of (leds_list, comment_name) in order of first appearance (unique segments only)
        max_led: maximum LED index found in input regions, or -1 if none
    """
    flat_leds: List[int] = []
    meta_entries: List[Tuple[int, int, int, str]] = []
    led_segments: List[Tuple[List[int], str]] = []

    # Map: exact LED list -> start_index in flat_leds
    start_index_by_leds: Dict[Tuple[int, ...], int] = {}

    max_led = -1

    for region in regions:
        region_id = int(region["regionId"])
        region_name = region.get("name", f"Region {region_id}")
        leds: List[int] = region.get("leds", [])

        if leds:
            max_led = max(max_led, max(leds))

        led_count = len(leds)

        # Validate led_count doesn't exceed uint8_t maximum (255)
        if led_count > 255:
            raise ValueError(
                f"Region {region_id} ('{region_name}') has {led_count} LEDs, "
                f"which exceeds the uint8_t limit of 255 in RegionLedMapMeta. "
                f"Consider splitting this region or using a larger field type."
            )

        if led_count == 0:
            start_index = 0
        else:
            key = tuple(int(x) for x in leds)
            existing = start_index_by_leds.get(key)
            if existing is not None:
                start_index = existing
            else:
                start_index = len(flat_leds)
                start_index_by_leds[key] = start_index
                flat_leds.extend(key)
                led_segments.append((list(key), region_name))

        meta_entries.append((region_id, start_index, led_count, region_name))

    return meta_entries, flat_leds, led_segments, max_led


def calculate_stats(regions: List[Dict]) -> Tuple[int, int, int, int, int, int]:
    """
    Calculate statistics for a region map.

    Returns:
        (num_regions, max_led_count, total_led_entries, meta_bytes, led_bytes, total_bytes)
    """
    num_regions = len(regions)

    # Deduplicated flat LED entries (real size of the generated LED array)
    _, flat_leds, _, max_led = build_flat_arrays(regions)
    total_led_entries = len(flat_leds)
    max_led_count = (max_led + 1) if max_led >= 0 else 0  # Convert from 0-indexed to count

    # Calculate memory sizes based on actual array entries
    # RegionLedMapMeta has 3 fields: region_id (uint16_t), start_offset (uint16_t), led_count (uint8_t)
    # = 2 + 2 + 1 = 5 bytes, but with padding it's likely 6 bytes per entry
    meta_bytes = num_regions * 5  # Conservative estimate
    led_bytes = total_led_entries * 2  # uint16_t array (actual entries in the array)
    total_bytes = meta_bytes + led_bytes

    return num_regions, max_led_count, total_led_entries, meta_bytes, led_bytes, total_bytes


def generate_metadata_array(map_name: str, regions: List[Dict]) -> List[str]:
    """Generate the metadata array entries with region name comments.

    Note: start offsets are computed with LED-list deduplication,
    so multiple regions may legally share the same start_index.
    """
    lines: List[str] = []
    meta_entries, _, _, _ = build_flat_arrays(regions)

    for region_id, start_index, led_count, region_name in meta_entries:
        lines.append(f"    {{{region_id:5}, {start_index:5}, {led_count:4}}},  // {region_name}")

    return lines


def generate_led_array(regions: List[Dict]) -> List[str]:
    """Generate the (deduplicated) flat LED positions array with comments."""
    lines: List[str] = []

    _, _, led_segments, _ = build_flat_arrays(regions)

    # First pass: generate all LED strings and find max length
    led_data: List[Tuple[str, str]] = []
    max_led_length = 0

    for leds, comment_name in led_segments:
        if not leds:
            continue

        led_str = ", ".join(str(led) for led in leds)
        led_line_with_comma = f"    {led_str},"
        max_led_length = max(max_led_length, len(led_line_with_comma))
        led_data.append((led_str, comment_name))

    # Second pass: add aligned comments
    for i, (led_str, comment_name) in enumerate(led_data):
        is_last = i == len(led_data) - 1

        led_line = f"    {led_str}" if is_last else f"    {led_str},"

        padding = max_led_length - len(led_line) + 2
        if padding < 2:
            padding = 2

        lines.append(led_line + " " * padding + f"// {comment_name}")

    return lines


def convert_map_name_to_const(map_name: str) -> str:
    """Convert map name to C++ constant style."""
    # Already in the right format, but make sure it's uppercase
    return map_name.upper()


def generate_header(data: Dict) -> str:
    """Generate the complete C++ header file."""
    lines = [
        "// AUTO-GENERATED FILE - DO NOT EDIT MANUALLY",
        "// Generated by tools/convert_region_map.py",
        "// Flat array структура для економії пам'яті",
        "",
        "#pragma once",
        '#include "JaamConfig.h"',
        "",
        "",
    ]

    maps = data["maps"]

    for map_name, map_data in maps.items():
        regions = map_data["regions"]

        # Calculate statistics
        num_regions, max_led_count, total_led_entries, meta_bytes, led_bytes, total_bytes = calculate_stats(regions)

        # Generate statistics comment
        lines.extend(
            [
                f"// Статистика для {map_name}:",
                f"//   Регіонів: {num_regions}",
                f"//   Фізичних LED: {max_led_count} (max index: {max_led_count - 1})",
                f"//   Записів у масиві: {total_led_entries}",
                f"//   Розмір метаданих: {meta_bytes} bytes",
                f"//   Розмір LED позицій: {led_bytes} bytes",
                f"//   Загальний розмір: {total_bytes} bytes",
                "",
            ]
        )

        # Generate size constant
        const_name = convert_map_name_to_const(map_name)
        lines.append(f"constexpr size_t {const_name}_SIZE = {num_regions};")
        lines.append("")

        # Generate metadata array
        meta_array_name = f"REGION_MAP_META_{map_name.replace('REGION_MAP_', '').replace('STATE_MAP_LED_', '')}"
        lines.append(f"const RegionLedMapMeta {meta_array_name}[] = {{")
        lines.extend(generate_metadata_array(map_name, regions))
        lines.append("};")
        lines.append("")

        # Generate LED positions array
        led_array_name = f"REGION_MAP_LEDS_{map_name.replace('REGION_MAP_', '').replace('STATE_MAP_LED_', '')}"
        lines.append(f"const uint16_t {led_array_name}[] = {{")
        lines.extend(generate_led_array(regions))
        lines.append("};")
        lines.append("")
        lines.append("")

    return "\n".join(lines)


def main():
    """Main entry point."""
    # Determine file paths
    script_dir = Path(__file__).parent

    if len(sys.argv) >= 2:
        input_file = Path(sys.argv[1])
    else:
        input_file = script_dir.parent / "data" / "region_maps.json"

    if len(sys.argv) >= 3:
        output_file = Path(sys.argv[2])
    else:
        output_file = script_dir.parent / "src" / "JaamConfig_Generated.h"

    print("=" * 60)
    print("Converting region maps to C++ header...")
    print("=" * 60)

    # Read input JSON
    print(f"\n📖 Reading: {input_file.name}")
    with open(input_file, "r", encoding="utf-8") as f:
        data = json.load(f)

    # Print statistics for each map
    maps = data["maps"]
    total_regions = 0
    total_physical_leds = 0
    total_led_entries = 0
    total_meta_bytes = 0
    total_led_bytes = 0

    for map_name, map_data in maps.items():
        regions = map_data["regions"]
        num_regions, max_led_count, led_entries, meta_bytes, led_bytes, total_bytes = calculate_stats(regions)

        total_regions += num_regions
        total_physical_leds += max_led_count
        total_led_entries += led_entries
        total_meta_bytes += meta_bytes
        total_led_bytes += led_bytes

        # Get short map name
        short_name = map_name.replace("REGION_MAP_", "").replace("STATE_MAP_LED_", "")

        print(f"\n🗺️  {short_name}")
        print(f"   Regions:      {num_regions:3d}")
        print(f"   Physical LEDs:{max_led_count:4d}")
        print(f"   LED entries:  {led_entries:4d}")
        print(f"   Metadata:     {meta_bytes:5d} bytes")
        print(f"   LED array:    {led_bytes:5d} bytes")
        print(f"   Total:        {total_bytes:5d} bytes")

    # Generate header content
    header_content = generate_header(data)

    # Write output file
    with open(output_file, "w", encoding="utf-8") as f:
        f.write(header_content)

    total_bytes = total_meta_bytes + total_led_bytes

    print("\n" + "=" * 60)
    print(f"✅ Generated: {output_file}")

    print("\n📊 Overall statistics:")
    print(f"   Maps:         {len(maps):3d}")
    avg_regions = total_regions // len(maps) if len(maps) > 0 else 0
    print(f"   Regions:      {total_regions:3d} (avg {avg_regions} per map)")
    print(f"   Physical LEDs:{total_physical_leds:4d} total")
    print(f"   LED entries:  {total_led_entries:4d} total")
    print(f"   Memory used:  {total_bytes:5d} bytes")
    print("=" * 60)


if __name__ == "__main__":
    main()
