#!/usr/bin/env python3
"""
Convert snowflake PNG to LVGL indexed format with palette
"""
from PIL import Image
import sys

def png_to_lvgl_indexed(png_path, output_name):
    """Convert PNG to LVGL indexed 1-bit format with palette"""

    img = Image.open(png_path)

    if img.mode != 'RGBA':
        img = img.convert('RGBA')

    width, height = img.size
    pixels = img.load()

    print(f"Converting {png_path}: {width}x{height} pixels")

    # Generate C header file
    header = f"""/*******************************************************************************
 * Snowflake image: {width} x {height} px
 * Format: Indexed 1-bit with 2-color palette
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

// Palette: 2 colors (transparent black, white)
const LV_ATTRIBUTE_MEM_ALIGN lv_color_t {output_name}_palette[] = {{
  LV_COLOR_MAKE(0x00, 0x00, 0x00),  // Index 0: Black (will be transparent)
  LV_COLOR_MAKE(0xff, 0xff, 0xff),  // Index 1: White
}};

// Pixel data: 1 bit per pixel (0=transparent, 1=white)
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_{output_name.upper()} uint8_t {output_name}_map[] = {{
"""

    # Convert pixels to 1-bit indexed
    pixel_data = []
    byte_val = 0
    bit_count = 0

    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            # If alpha > 128 and pixel is bright, it's white (1), else transparent (0)
            bit = 1 if (a > 128 and r > 128) else 0
            byte_val = (byte_val << 1) | bit
            bit_count += 1

            if bit_count == 8:
                pixel_data.append(f"0x{byte_val:02x}")
                byte_val = 0
                bit_count = 0

    # Handle remaining bits
    if bit_count > 0:
        byte_val = byte_val << (8 - bit_count)
        pixel_data.append(f"0x{byte_val:02x}")

    # Format into rows
    pixel_rows = []
    for i in range(0, len(pixel_data), 16):
        row = pixel_data[i:i+16]
        pixel_rows.append("  " + ", ".join(row) + ",")

    header += "\n".join(pixel_rows)
    header = header.rstrip(",")

    header += f"""
}};

const lv_img_dsc_t {output_name} = {{
  .header.cf = LV_IMG_CF_INDEXED_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = {width},
  .header.h = {height},
  .data_size = sizeof({output_name}_map) + sizeof({output_name}_palette),
  .data = {output_name}_map,
}};
"""

    return header

if __name__ == "__main__":
    # Convert 12px version
    png_file = "/Users/danielblock/Projects/Stonecold/snowflake_12px.png"
    output_file = "/Users/danielblock/Projects/Stonecold/src/snowflake_img.c"
    output_name = "snowflake_img"

    c_code = png_to_lvgl_indexed(png_file, output_name)

    with open(output_file, "w") as f:
        f.write(c_code)

    print(f"✓ Generated {output_file}")
    print(f"  Usage: lv_img_set_src(img, &{output_name});")
