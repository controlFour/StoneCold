#!/usr/bin/env python3
"""
Generate larger Montserrat fonts for LVGL
Uses the LVGL online font converter API
"""
import requests
import json
import sys

def generate_lvgl_font(font_size, output_name):
    """
    Generate LVGL font using the online converter API
    Font sizes: 60, 72, 84, 96, 108, 120
    """

    print(f"Generating {output_name} (Montserrat {font_size}pt)...")

    # LVGL font converter API endpoint
    url = "https://lvgl.io/tools/fontconverter"

    # Configuration for font generation
    payload = {
        'name': output_name,
        'size': font_size,
        'bpp': 4,  # 4 bits per pixel for antialiasing
        'format': 'lvgl',
        'font': 'montserrat',  # Use Montserrat font
        'range': '0x20-0x7F,0xB0',  # Basic ASCII + degree symbol
        'compression': 'none'
    }

    print(f"  Requesting font data from LVGL converter...")
    print(f"  Size: {font_size}pt, Range: ASCII + degree symbol (°)")

    # Note: The actual LVGL online converter doesn't have a direct API
    # We'll need to use it manually or generate the font data ourselves

    print("\n⚠️  The LVGL font converter needs to be used manually:")
    print(f"\n1. Visit: https://lvgl.io/tools/fontconverter")
    print(f"2. Settings:")
    print(f"   - Name: {output_name}")
    print(f"   - Size: {font_size} px")
    print(f"   - Bpp: 4 bit-per-pixel")
    print(f"   - Font: Upload Montserrat (or use built-in)")
    print(f"   - Range: 0x20-0x7F,0xB0")
    print(f"   - Output format: C array")
    print(f"3. Click 'Convert' and download the .c file")
    print(f"4. Save as: src/{output_name}.c")
    print()

def main():
    # Generate fonts for common large sizes
    sizes = [
        (60, "lv_font_montserrat_60"),
        (72, "lv_font_montserrat_72"),
        (84, "lv_font_montserrat_84"),
        (96, "lv_font_montserrat_96"),
    ]

    print("=" * 70)
    print("LVGL Large Font Generator")
    print("=" * 70)
    print()

    for size, name in sizes:
        generate_lvgl_font(size, name)

    print("=" * 70)
    print("\nAlternatively, I can create the font files directly for you.")
    print("The LVGL font converter is a web tool that generates C code.")
    print()

if __name__ == "__main__":
    main()
