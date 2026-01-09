#include "snow_effect.h"
#include <stdlib.h>
#include <math.h>

// Configuration
#define SNOWFLAKE_COUNT 20          // Number of snowflakes
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#define UPDATE_PERIOD_MS 30         // ~33 FPS
#define WIND_CHANGE_SPEED 0.005f    // How fast wind bias changes

// Snowflake properties
typedef struct {
    lv_obj_t *canvas;       // LVGL canvas for drawing snowflake shape
    lv_color_t *cbuf;       // Canvas buffer
    float x;                // X position (float for smooth sub-pixel movement)
    float y;                // Y position
    float speed;            // Fall speed (pixels per frame)
    float drift_speed;      // Horizontal drift speed
    float drift_phase;      // Phase offset for sine wave drift
    float rotation;         // Current rotation angle (0-3600, LVGL uses 0.1 degree units)
    float rotation_speed;   // Rotation speed per frame
    uint8_t size;           // Snowflake diameter
    lv_opa_t opacity;       // Opacity
} snowflake_t;

// Module state
static snowflake_t snowflakes[SNOWFLAKE_COUNT];
static float wind_bias = 0.0f;
static float wind_target = 0.0f;
static uint32_t frame_count = 0;
static bool is_paused = false;

// Forward declarations
static void update_snow(void);
static void init_snowflake(snowflake_t *flake, lv_obj_t *parent, bool randomize_y);
static float randf(float min, float max);
static int randi(int min, int max);

/**
 * Random float between min and max
 */
static float randf(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

/**
 * Random integer between min and max (inclusive)
 */
static int randi(int min, int max) {
    return min + (rand() % (max - min + 1));
}

/**
 * Draw a 6-armed snowflake on a canvas with blue outline
 */
static void draw_snowflake_shape(lv_obj_t *canvas, uint8_t canvas_size, uint8_t arm_length) {
    // Clear canvas with transparent background
    lv_canvas_fill_bg(canvas, lv_color_hex(0x1a1a1a), LV_OPA_TRANSP);

    int16_t center = canvas_size / 2;

    // FIRST PASS: Draw blue outline (thicker lines)
    lv_draw_line_dsc_t blue_line_dsc;
    lv_draw_line_dsc_init(&blue_line_dsc);
    blue_line_dsc.color = lv_color_hex(0x4488ff);  // Light blue
    blue_line_dsc.width = 4;  // Thicker for outline
    blue_line_dsc.opa = LV_OPA_COVER;

    // Draw 6 arms with blue outline
    for (int i = 0; i < 6; i++) {
        float angle = i * 60.0f * 3.14159f / 180.0f;

        // Main arm (blue)
        lv_point_t points[2];
        points[0].x = center;
        points[0].y = center;
        points[1].x = center + (int16_t)(arm_length * cosf(angle));
        points[1].y = center + (int16_t)(arm_length * sinf(angle));
        lv_canvas_draw_line(canvas, points, 2, &blue_line_dsc);

        // Small branches at 60% length (blue)
        int16_t branch_x = center + (int16_t)(arm_length * 0.6f * cosf(angle));
        int16_t branch_y = center + (int16_t)(arm_length * 0.6f * sinf(angle));

        lv_draw_line_dsc_t blue_branch_dsc;
        lv_draw_line_dsc_init(&blue_branch_dsc);
        blue_branch_dsc.color = lv_color_hex(0x4488ff);
        blue_branch_dsc.width = 3;  // Thicker for outline
        blue_branch_dsc.opa = LV_OPA_COVER;

        // Two side branches
        for (int b = -1; b <= 1; b += 2) {
            float branch_angle = angle + b * 45.0f * 3.14159f / 180.0f;
            lv_point_t branch_points[2];
            branch_points[0].x = branch_x;
            branch_points[0].y = branch_y;
            branch_points[1].x = branch_x + (int16_t)(3 * cosf(branch_angle));
            branch_points[1].y = branch_y + (int16_t)(3 * sinf(branch_angle));
            lv_canvas_draw_line(canvas, branch_points, 2, &blue_branch_dsc);
        }
    }

    // SECOND PASS: Draw white snowflake on top
    lv_draw_line_dsc_t white_line_dsc;
    lv_draw_line_dsc_init(&white_line_dsc);
    white_line_dsc.color = lv_color_hex(0xffffff);
    white_line_dsc.width = 2;
    white_line_dsc.opa = LV_OPA_COVER;

    // Draw 6 arms in white
    for (int i = 0; i < 6; i++) {
        float angle = i * 60.0f * 3.14159f / 180.0f;

        // Main arm (white)
        lv_point_t points[2];
        points[0].x = center;
        points[0].y = center;
        points[1].x = center + (int16_t)(arm_length * cosf(angle));
        points[1].y = center + (int16_t)(arm_length * sinf(angle));
        lv_canvas_draw_line(canvas, points, 2, &white_line_dsc);

        // Small branches at 60% length (white)
        int16_t branch_x = center + (int16_t)(arm_length * 0.6f * cosf(angle));
        int16_t branch_y = center + (int16_t)(arm_length * 0.6f * sinf(angle));

        lv_draw_line_dsc_t white_branch_dsc;
        lv_draw_line_dsc_init(&white_branch_dsc);
        white_branch_dsc.color = lv_color_hex(0xffffff);
        white_branch_dsc.width = 1;
        white_branch_dsc.opa = LV_OPA_COVER;

        // Two side branches
        for (int b = -1; b <= 1; b += 2) {
            float branch_angle = angle + b * 45.0f * 3.14159f / 180.0f;
            lv_point_t branch_points[2];
            branch_points[0].x = branch_x;
            branch_points[0].y = branch_y;
            branch_points[1].x = branch_x + (int16_t)(3 * cosf(branch_angle));
            branch_points[1].y = branch_y + (int16_t)(3 * sinf(branch_angle));
            lv_canvas_draw_line(canvas, branch_points, 2, &white_branch_dsc);
        }
    }

    // Center dot with blue outline
    lv_draw_rect_dsc_t blue_rect_dsc;
    lv_draw_rect_dsc_init(&blue_rect_dsc);
    blue_rect_dsc.bg_color = lv_color_hex(0x4488ff);
    blue_rect_dsc.bg_opa = LV_OPA_COVER;
    blue_rect_dsc.radius = LV_RADIUS_CIRCLE;
    blue_rect_dsc.border_width = 0;
    lv_canvas_draw_rect(canvas, center - 2, center - 2, 5, 5, &blue_rect_dsc);

    // Center dot (white on top)
    lv_draw_rect_dsc_t white_rect_dsc;
    lv_draw_rect_dsc_init(&white_rect_dsc);
    white_rect_dsc.bg_color = lv_color_hex(0xffffff);
    white_rect_dsc.bg_opa = LV_OPA_COVER;
    white_rect_dsc.radius = LV_RADIUS_CIRCLE;
    white_rect_dsc.border_width = 0;
    lv_canvas_draw_rect(canvas, center - 1, center - 1, 3, 3, &white_rect_dsc);
}

/**
 * Initialize a single snowflake with random properties
 */
static void init_snowflake(snowflake_t *flake, lv_obj_t *parent, bool randomize_y) {
    // Position: random X, either top or random Y
    flake->x = randf(-10, SCREEN_WIDTH + 10);
    if (randomize_y) {
        flake->y = randf(-20, SCREEN_HEIGHT);
    } else {
        flake->y = randf(-50, -10);  // Start above screen
    }

    // Speed: slower = farther away (smaller, more transparent)
    flake->speed = randf(0.8f, 2.5f);

    // Size based on speed (depth): slower = smaller/farther
    float depth_factor = (flake->speed - 0.8f) / 1.7f;  // 0.0 to 1.0
    uint8_t arm_length = (uint8_t)(4 + depth_factor * 6);  // 4 to 10 pixel arms
    flake->size = arm_length * 2 + 6;  // Canvas size with padding

    // Opacity based on depth: farther = more transparent
    flake->opacity = (lv_opa_t)(180 + depth_factor * 75);  // 180 to 255

    // Drift properties
    flake->drift_speed = randf(0.3f, 1.0f);
    flake->drift_phase = randf(0, 6.28f);  // Random phase (0 to 2π)

    // Rotation
    flake->rotation = randf(0, 3600);
    flake->rotation_speed = randf(-20, 20);

    // Create or recreate canvas
    if (flake->canvas != NULL) {
        lv_obj_del(flake->canvas);
        if (flake->cbuf != NULL) {
            free(flake->cbuf);
        }
    }

    // Allocate canvas buffer
    flake->cbuf = (lv_color_t*)malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(flake->size, flake->size));

    // Create canvas
    flake->canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(flake->canvas, flake->cbuf, flake->size, flake->size, LV_IMG_CF_TRUE_COLOR);

    // Style canvas (transparent background, no borders)
    lv_obj_set_style_bg_opa(flake->canvas, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(flake->canvas, 0, 0);
    lv_obj_set_style_pad_all(flake->canvas, 0, 0);

    // Draw snowflake shape
    draw_snowflake_shape(flake->canvas, flake->size, arm_length);

    // Make non-interactive
    lv_obj_clear_flag(flake->canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(flake->canvas, LV_OBJ_FLAG_SCROLLABLE);

    // Move to background
    lv_obj_move_background(flake->canvas);

    // Set position and opacity
    lv_obj_set_pos(flake->canvas, (lv_coord_t)flake->x, (lv_coord_t)flake->y);
    lv_obj_set_style_opa(flake->canvas, flake->opacity, 0);
}

/**
 * Update snow animation (called manually from main loop)
 */
static void update_snow(void) {
    frame_count++;

    // Slowly change wind bias target
    if (frame_count % 100 == 0) {
        wind_target = randf(-1.5f, 1.5f);
    }

    // Smoothly interpolate current wind toward target
    wind_bias += (wind_target - wind_bias) * WIND_CHANGE_SPEED;

    // Update each snowflake
    for (int i = 0; i < SNOWFLAKE_COUNT; i++) {
        snowflake_t *flake = &snowflakes[i];

        // Update Y position (falling)
        flake->y += flake->speed;

        // Update X position (drift + wind)
        float drift_offset = sinf((frame_count * 0.02f) + flake->drift_phase) * flake->drift_speed;
        flake->x += drift_offset + wind_bias * 0.15f;

        // Update rotation (for future use with shaped snowflakes)
        flake->rotation += flake->rotation_speed;
        if (flake->rotation >= 3600) flake->rotation -= 3600;
        if (flake->rotation < 0) flake->rotation += 3600;

        // Check if snowflake has left the screen
        if (flake->y > SCREEN_HEIGHT + 20 || flake->x < -30 || flake->x > SCREEN_WIDTH + 30) {
            // Simple respawn without recreating canvas
            flake->y = randf(-50, -10);
            flake->x = randf(-10, SCREEN_WIDTH + 10);
            flake->speed = randf(0.8f, 2.5f);
        }

        // Update canvas position
        lv_obj_set_pos(flake->canvas, (lv_coord_t)flake->x, (lv_coord_t)flake->y);
    }
}

/**
 * Initialize the snow effect
 */
void snow_effect_init(lv_obj_t *parent) {
    if (parent == NULL) {
        return;
    }

    // Initialize random seed based on time
    // Note: For better randomness, could use millis() as seed

    // Initialize all snowflakes
    for (int i = 0; i < SNOWFLAKE_COUNT; i++) {
        snowflakes[i].canvas = NULL;
        snowflakes[i].cbuf = NULL;
        init_snowflake(&snowflakes[i], parent, true);  // Randomize Y for initial spread
    }

    // Initialize wind and frame counter
    wind_bias = 0.0f;
    wind_target = randf(-1.0f, 1.0f);
    frame_count = 0;
}

/**
 * Start the snow animation
 * Note: Animation is driven by snow_effect_manual_update() calls from main loop
 */
void snow_effect_start(void) {
    // Animation runs via manual updates in main loop
}

/**
 * Stop the snow animation
 * Note: To stop, simply don't call snow_effect_manual_update()
 */
void snow_effect_stop(void) {
    // Animation runs via manual updates in main loop
}

/**
 * Clean up snow effect resources
 */
void snow_effect_deinit(void) {
    // Delete all snowflake canvases and buffers
    for (int i = 0; i < SNOWFLAKE_COUNT; i++) {
        if (snowflakes[i].canvas != NULL) {
            lv_obj_del(snowflakes[i].canvas);
            snowflakes[i].canvas = NULL;
        }
        if (snowflakes[i].cbuf != NULL) {
            free(snowflakes[i].cbuf);
            snowflakes[i].cbuf = NULL;
        }
    }
}

/**
 * Manually trigger one snow update (called from main loop)
 */
void snow_effect_manual_update(void) {
    if (!is_paused) {
        update_snow();
    }
}

/**
 * Pause snow effect and hide snowflakes
 */
void snow_effect_pause_and_hide(void) {
    is_paused = true;
    // Hide all snowflakes
    for (int i = 0; i < SNOWFLAKE_COUNT; i++) {
        if (snowflakes[i].canvas != NULL) {
            lv_obj_add_flag(snowflakes[i].canvas, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/**
 * Resume snow effect and show snowflakes
 */
void snow_effect_resume_and_show(void) {
    is_paused = false;
    // Show all snowflakes
    for (int i = 0; i < SNOWFLAKE_COUNT; i++) {
        if (snowflakes[i].canvas != NULL) {
            lv_obj_clear_flag(snowflakes[i].canvas, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/**
 * Check if snow effect is currently paused
 */
bool snow_effect_is_paused(void) {
    return is_paused;
}
