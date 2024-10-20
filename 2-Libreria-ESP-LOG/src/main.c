#include <stdio.h>
#include "driver/gpio.h"            
#include "freertos/FreeRTOS.h"      
#include "freertos/task.h"          
#include "esp_log.h"

#define led1 8                      

static const char* TAG = "main"; //permite indicar en el modulo donde se invoca el codigo. en este caso el Tag es main.

uint8_t led_level = 0;              

uint8_t count=0;

esp_err_t init_led(void);           
esp_err_t blink_led(void);          
                                    
void app_main(void)
{

    init_led();

    while(1){

        vTaskDelay(500 / portTICK_PERIOD_MS);

        blink_led();
        
        count += 1;
        if(count > 30){
            count = 0;
        }
        if(count<10)
        {
            //imprimir información en color verde (información)
            ESP_LOGI(TAG, "Value: %u", count); 
        }
        if (count >= 10 && count < 20)
        {
            //imprimir informacion de color amarillo (advertencia)
            ESP_LOGW(TAG, "Value: %u", count);
        }
        if (count >=20)
        {
            //imprimir información de color rojo (error)
            ESP_LOGE(TAG, "Value: %u", count);
        }

    }
}

esp_err_t init_led(void)
{
    gpio_reset_pin(led1);
    gpio_set_direction(led1, GPIO_MODE_OUTPUT);
    return ESP_OK;
}
esp_err_t blink_led(void)
{
    led_level = !led_level;
    gpio_set_level(led1,led_level);
    return ESP_OK;

}
