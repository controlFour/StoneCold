#!/usr/bin/env python3
"""
Generate a realistic snowflake PNG
"""
from PIL import Image, ImageDraw
import math

def create_realistic_snowflake(size=16):
    """Create a realistic 6-armed snowflake"""
    # Create transparent image
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    center = size / 2

    # Draw 6 main arms radiating from center
    arm_length = size / 2 - 1

    for i in range(6):
        angle = i * 60 * math.pi / 180

        # Main arm
        x_end = center + arm_length * math.cos(angle)
        y_end = center + arm_length * math.sin(angle)
        draw.line([center, center, x_end, y_end], fill=(255, 255, 255, 255), width=2)

        # Two side branches at 1/3 length
        branch_dist = arm_length * 0.4
        x_branch = center + branch_dist * math.cos(angle)
        y_branch = center + branch_dist * math.sin(angle)

        for branch_angle_offset in [-45, 45]:
            branch_angle = angle + branch_angle_offset * math.pi / 180
            branch_len = 2.5
            x_b_end = x_branch + branch_len * math.cos(branch_angle)
            y_b_end = y_branch + branch_len * math.sin(branch_angle)
            draw.line([x_branch, y_branch, x_b_end, y_b_end],
                     fill=(255, 255, 255, 255), width=1)

        # Two side branches at 2/3 length
        branch_dist2 = arm_length * 0.7
        x_branch2 = center + branch_dist2 * math.cos(angle)
        y_branch2 = center + branch_dist2 * math.sin(angle)

        for branch_angle_offset in [-45, 45]:
            branch_angle = angle + branch_angle_offset * math.pi / 180
            branch_len = 2
            x_b_end = x_branch2 + branch_len * math.cos(branch_angle)
            y_b_end = y_branch2 + branch_len * math.sin(branch_angle)
            draw.line([x_branch2, y_branch2, x_b_end, y_b_end],
                     fill=(255, 255, 255, 255), width=1)

    # Center dot
    draw.ellipse([center-1.5, center-1.5, center+1.5, center+1.5],
                 fill=(255, 255, 255, 255))

    return img

if __name__ == "__main__":
    for size in [12, 16]:
        img = create_realistic_snowflake(size)
        filename = f"/Users/danielblock/Projects/Stonecold/snowflake_{size}px.png"
        img.save(filename)
        print(f"✓ Generated {filename}")

        # Check pixel count
        pixels = img.load()
        opaque = sum(1 for y in range(size) for x in range(size) if pixels[x,y][3] > 128)
        print(f"  Size: {size}x{size}, Opaque pixels: {opaque}/{size*size}")
