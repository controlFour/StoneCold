#!/usr/bin/env python3
"""
Generate a simple snowflake image
"""
from PIL import Image, ImageDraw

def create_snowflake(size=16):
    """Create a simple snowflake shape"""
    # Create transparent image
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    center = size // 2

    # Draw a simple snowflake with 6 arms
    # Center dot
    draw.ellipse([center-2, center-2, center+2, center+2], fill=(255, 255, 255, 255))

    # Draw 6 arms radiating from center
    import math
    arm_length = size // 2 - 2

    for i in range(6):
        angle = i * 60 * math.pi / 180
        x_end = center + arm_length * math.cos(angle)
        y_end = center + arm_length * math.sin(angle)

        # Main arm
        draw.line([center, center, x_end, y_end], fill=(255, 255, 255, 255), width=2)

        # Small branches at 2/3 length
        branch_len = 3
        branch_pos = 0.6
        x_branch = center + arm_length * branch_pos * math.cos(angle)
        y_branch = center + arm_length * branch_pos * math.sin(angle)

        # Two small branches per arm
        for branch_angle in [angle - 45 * math.pi / 180, angle + 45 * math.pi / 180]:
            x_b_end = x_branch + branch_len * math.cos(branch_angle)
            y_b_end = y_branch + branch_len * math.sin(branch_angle)
            draw.line([x_branch, y_branch, x_b_end, y_b_end], fill=(255, 255, 255, 255), width=1)

    return img

if __name__ == "__main__":
    img = create_snowflake(16)
    img.save("/Users/danielblock/Projects/Stonecold/snowflake.png")
    print("✓ Generated snowflake.png (16x16)")
