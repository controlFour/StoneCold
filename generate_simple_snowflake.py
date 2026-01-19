#!/usr/bin/env python3
"""
Generate a very simple snowflake - just a white circle
"""
from PIL import Image, ImageDraw

def create_simple_snowflake(size=12):
    """Create a simple circular snowflake"""
    # Create transparent image
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    center = size // 2
    radius = size // 2 - 2

    # Draw a simple white circle
    draw.ellipse([center-radius, center-radius, center+radius, center+radius],
                 fill=(255, 255, 255, 255))

    return img

if __name__ == "__main__":
    img = create_simple_snowflake(12)
    img.save("/Users/danielblock/Projects/Stonecold/snowflake_simple.png")
    print("✓ Generated snowflake_simple.png (12x12 circle)")

    # Check what we created
    pixels = img.load()
    opaque = sum(1 for y in range(12) for x in range(12) if pixels[x,y][3] > 128)
    print(f"  Opaque pixels: {opaque}/144")
