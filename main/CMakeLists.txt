idf_component_register(SRCS
    "main.c"
    "fonts/Rubik_Regular_36.c"
    "fonts/Rubik_Medium_48.c"
    INCLUDE_DIRS "."
    REQUIRES 
    lvgl_weact_epaper 
    lvgl__lvgl 
    esp_timer)
