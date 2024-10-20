/*falta documentar*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include "esp_log.h"

#define GPIO_WS2812B 8
#define MAX_LEDS 1

static const char *TAG = "LED STRIP";
static led_strip_t *pStrip_a;

static void set_rgb_led(void)
{
    pStrip_a = led_strip_init(0,GPIO_WS2812B, MAX_LEDS);
    pStrip_a->clear(pStrip_a,100);
}

void app_main(void)
{
    set_rgb_led();

    while (true)
    {
        ESP_LOGI(TAG, "LED RED");
        pStrip_a->set_pixel(pStrip_a,0,255,0,0);
        pStrip_a->refresh(pStrip_a,100);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "LED GREEN");
        pStrip_a->set_pixel(pStrip_a,0,0,255,0);
        pStrip_a->refresh(pStrip_a,100);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "LED BLUE");
        pStrip_a->set_pixel(pStrip_a,0,0,0,255);
        pStrip_a->refresh(pStrip_a,100);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
