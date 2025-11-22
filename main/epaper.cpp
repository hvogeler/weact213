#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"

extern "C" void app_main(void)
{
    while (1)
    {
        ESP_LOGI("EPAPER", "Hello, ePaper!");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
