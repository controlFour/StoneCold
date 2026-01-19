#!/usr/bin/env python3
"""
Convert PNG to LVGL C array format
"""
from PIL import Image
import sys

def png_to_lvgl_c_array(png_path, output_name, target_size=30):
    """Convert PNG to LVGL C array with alpha channel support (RGB565A8 format)"""

    # Load the image
    img = Image.open(png_path)

    # Resize to target size with high-quality resampling
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

    # Convert pixels to C array (ALPHA_8BIT format)
    # First pass: find luminance range
    min_lum = 255
    max_lum = 0
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            luminance = int(0.299 * r + 0.587 * g + 0.114 * b)
            min_lum = min(min_lum, luminance)
            max_lum = max(max_lum, luminance)

    # Second pass: convert with remapping and threshold
    pixel_data = []
    lum_range = max_lum - min_lum
    if lum_range == 0:
        lum_range = 1  # Avoid division by zero

    # Threshold: pixels below this percentage of the range become fully transparent
    threshold_percent = 0.15  # Bottom 15% becomes transparent

    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            # Calculate luminance (perceived brightness)
            luminance = int(0.299 * r + 0.587 * g + 0.114 * b)

            # Apply threshold to cut off dark background pixels
            if luminance < min_lum + (lum_range * threshold_percent):
                alpha = 0  # Fully transparent
            else:
                # Remap remaining range to 0-255
                adjusted_lum = luminance - min_lum - (lum_range * threshold_percent)
                adjusted_range = lum_range * (1 - threshold_percent)
                alpha = int(adjusted_lum * 255 / adjusted_range)
                alpha = min(255, max(0, alpha))  # Clamp to 0-255

            pixel_data.append(f"0x{alpha:02x}")

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
    if len(sys.argv) >= 3:
        png_file = sys.argv[1]
        output_file = sys.argv[2]
        output_name = sys.argv[3] if len(sys.argv) >= 4 else "image"
        target_size = int(sys.argv[4]) if len(sys.argv) >= 5 else None
    else:
        # Default: convert settings.png
        png_file = "/Users/danielblock/Projects/Stonecold/settings.png"
        output_file = "/Users/danielblock/Projects/Stonecold/src/settings_img.c"
        output_name = "settings_img"
        target_size = 30

    if target_size:
        c_code = png_to_lvgl_c_array(png_file, output_name, target_size)
    else:
        c_code = png_to_lvgl_c_array(png_file, output_name)

    with open(output_file, "w") as f:
        f.write(c_code)

    print(f"✓ Generated {output_file}")
    print(f"  Add this to your code: LV_IMG_DECLARE({output_name});")
    print(f"  Then use: lv_img_set_src(img, &{output_name});")
