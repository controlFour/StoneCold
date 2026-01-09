#!/usr/bin/env python3
"""
Convert PNG with alpha channel to LVGL C array format
Uses alpha channel directly (not luminance conversion)
"""
from PIL import Image
import sys

def png_alpha_to_lvgl_c_array(png_path, output_name, target_size=None):
    """Convert PNG to LVGL C array using alpha channel directly"""

    # Load the image
    img = Image.open(png_path)

    # Resize if needed
    if target_size:
        img = img.resize((target_size, target_size), Image.Resampling.LANCZOS)

    # Convert to RGBA if not already
    if img.mode != 'RGBA':
        img = img.convert('RGBA')

    width, height = img.size
    pixels = img.load()

    print(f"Converting {png_path}: {width}x{height} pixels")

    # Generate C header file
    header = f"""/*******************************************************************************
 * Size: {width} x {height} px
 * Bit depth: ALPHA_8BIT (alpha-only, for recoloring)
 *******************************************************************************/

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_{output_name.upper()}
#define LV_ATTRIBUTE_IMG_{output_name.upper()}
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_{output_name.upper()} uint8_t {output_name}_map[] = {{
"""

    # Convert pixels to C array - just use alpha channel directly
    pixel_data = []
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            # Use alpha directly
            pixel_data.append(f"0x{a:02x}")

    # Format into rows
    pixel_rows = []
    for i in range(0, len(pixel_data), 16):  # 16 bytes per row
        row = pixel_data[i:i+16]
        pixel_rows.append("  " + ", ".join(row) + ",")

    header += "\n".join(pixel_rows)
    header = header.rstrip(",")  # Remove trailing comma

    header += f"""
}};

const lv_img_dsc_t {output_name} = {{
  .header.cf = LV_IMG_CF_ALPHA_8BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = {width},
  .header.h = {height},
  .data_size = {width * height},
  .data = {output_name}_map,
}};
"""

    return header

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 convert_alpha_png_to_c.py <input.png> <output.c> <name> [size]")
        sys.exit(1)

    png_file = sys.argv[1]
    output_file = sys.argv[2]
    output_name = sys.argv[3]
    target_size = int(sys.argv[4]) if len(sys.argv) >= 5 else None

    c_code = png_alpha_to_lvgl_c_array(png_file, output_name, target_size)

    with open(output_file, "w") as f:
        f.write(c_code)

    print(f"✓ Generated {output_file}")
    print(f"  Add this to your code: LV_IMG_DECLARE({output_name});")
    print(f"  Then use: lv_img_set_src(img, &{output_name});")
