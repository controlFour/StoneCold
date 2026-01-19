#!/usr/bin/env python3
"""
Generate large LVGL font files from TTF
Requires: pip install lv_font_conv pillow freetype-py
"""
import subprocess
import sys
import os

def check_lv_font_conv():
    """Check if lv_font_conv is installed"""
    try:
        result = subprocess.run(['lv_font_conv', '--help'],
                              capture_output=True, text=True)
        return True
    except FileNotFoundError:
        return False

def generate_font(size, output_path):
    """Generate LVGL font using lv_font_conv"""

    font_name = f"lv_font_montserrat_{size}"
    output_file = f"{output_path}/{font_name}.c"

    # Use system Montserrat font or download one
    # On macOS, system fonts are in /System/Library/Fonts/ or /Library/Fonts/
    font_paths = [
        "/System/Library/Fonts/Supplemental/Montserrat-Regular.ttf",
        "/Library/Fonts/Montserrat-Regular.ttf",
        "Montserrat-Regular.ttf"
    ]

    font_file = None
    for path in font_paths:
        if os.path.exists(path):
            font_file = path
            break

    if not font_file:
        print(f"❌ Montserrat font not found. Downloading...")
        # Download Montserrat from Google Fonts
        import urllib.request
        url = "https://github.com/JulietaUla/Montserrat/raw/master/fonts/ttf/Montserrat-Regular.ttf"
        font_file = "Montserrat-Regular.ttf"
        urllib.request.urlretrieve(url, font_file)
        print(f"✓ Downloaded Montserrat font")

    print(f"Generating {font_name} from {font_file}...")

    # lv_font_conv command
    cmd = [
        'lv_font_conv',
        '--no-compress',
        '--no-prefilter',
        '--bpp', '4',
        '--size', str(size),
        '--font', font_file,
        '-r', '0x20-0x7F',  # Basic ASCII
        '-r', '0xB0',       # Degree symbol
        '--format', 'lvgl',
        '--force-fast-kern-format',
        '-o', output_file
    ]

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        print(f"✓ Generated {output_file}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"❌ Error generating font: {e}")
        print(f"stderr: {e.stderr}")
        return False

def main():
    print("=" * 70)
    print("LVGL Large Font Generator")
    print("=" * 70)
    print()

    # Check if lv_font_conv is installed
    if not check_lv_font_conv():
        print("❌ lv_font_conv not found!")
        print("\nTo install:")
        print("  npm install -g lv_font_conv")
        print("\nOr use the online converter:")
        print("  https://lvgl.io/tools/fontconverter")
        sys.exit(1)

    # Create src directory if it doesn't exist
    os.makedirs("src", exist_ok=True)

    # Generate fonts
    sizes = [60, 72, 84, 96]

    for size in sizes:
        if not generate_font(size, "src"):
            print(f"⚠️  Skipping {size}pt")
            continue

    print("\n" + "=" * 70)
    print("✓ Font generation complete!")
    print("\nNext steps:")
    print("1. Include the generated font files in your main.cpp:")
    print("   LV_FONT_DECLARE(lv_font_montserrat_60);")
    print("2. Use the font:")
    print("   lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_60, 0);")
    print("=" * 70)

if __name__ == "__main__":
    main()
