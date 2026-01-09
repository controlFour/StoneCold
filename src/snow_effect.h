#ifndef SNOW_EFFECT_H
#define SNOW_EFFECT_H

#include "lvgl.h"

/**
 * Initialize the snow effect (creates snowflakes)
 * Call this after LVGL is initialized and you have a screen set up
 * @param parent The parent object (usually the screen) where snow will be drawn
 */
void snow_effect_init(lv_obj_t *parent);

/**
 * Start the snow animation
 * The effect starts automatically after init, but you can use this to restart it
 */
void snow_effect_start(void);

/**
 * Stop the snow animation
 */
void snow_effect_stop(void);

/**
 * Clean up snow effect resources
 */
void snow_effect_deinit(void);

/**
 * Manually trigger one snow update (called from main loop)
 */
void snow_effect_manual_update(void);

/**
 * Pause snow effect and hide snowflakes
 */
void snow_effect_pause_and_hide(void);

/**
 * Resume snow effect and show snowflakes
 */
void snow_effect_resume_and_show(void);

/**
 * Check if snow effect is currently paused
 */
bool snow_effect_is_paused(void);

#endif /* SNOW_EFFECT_H */
