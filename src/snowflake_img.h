#ifndef SNOWFLAKE_IMG_H
#define SNOWFLAKE_IMG_H

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

#ifndef LV_ATTRIBUTE_IMG_SNOWFLAKE_IMG
#define LV_ATTRIBUTE_IMG_SNOWFLAKE_IMG
#endif

LV_IMG_DECLARE(snowflake_img);

#endif /* SNOWFLAKE_IMG_H */
