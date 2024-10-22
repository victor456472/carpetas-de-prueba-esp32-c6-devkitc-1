#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include "driver/ledc.h" // libreria para PWM
#include "driver/adc.h" //libreria de entradas analogicas

static const char *TAG = "main";
uint8_t led_level = 0;

esp_err_t set_timer(void);
esp_err_t set_adc(void); //funcion para configurar el ADC 

/*timers*/

TimerHandle_t xTimers;
int interval = 100;
int timerId = 1;

/*ADC*/

int adc_val = 0; // variable que alacena la lectura del ADC


void vTimerCallback(TimerHandle_t pxTimer)
{
    adc_val =  adc1_get_raw(ADC1_CHANNEL_4); //lectura del ADC del canal 4
    printf("lectura adc: %u\n", adc_val); // se imprime en pantalla la lectura del ADC
}

void app_main(void)
{
    set_adc();
    set_timer();
}

esp_err_t set_timer(void)
{
    ESP_LOGI(TAG, "Timer init configuration");
    xTimers = xTimerCreate("Timer",                   
                           (pdMS_TO_TICKS(interval)), 
                           pdTRUE,                   
                           (void *)timerId,           
                           vTimerCallback             
    );

    if (xTimers == NULL)
    {
        ESP_LOGE(TAG, "The timer was not created.");
    }
    else
    {

        if (xTimerStart(xTimers, 0) != pdPASS)
        { 
            ESP_LOGE(TAG, "The timer could not be set into the Active state.");
        }
    }
    return ESP_OK;
}
esp_err_t set_adc(void)
{
/*
con el metodo adc1_config_channel_atten() se configura el ADC. los
parametros requeridos son:

1) canal del ADC. hay que tene en cuenta que estos se asignan a un´
GPIO correspondiente dependiendo de la placa. para la ESP32-C6-DevKitC-1
los canales correspondientes a cada GPIO se lista a continuación:

    ADC1_CHANNEL_0,   ADC1 channel 0 is GPIO0 
    ADC1_CHANNEL_1,   ADC1 channel 1 is GPIO1 
    ADC1_CHANNEL_2,   ADC1 channel 2 is GPIO2 
    ADC1_CHANNEL_3,   ADC1 channel 3 is GPIO3 
    ADC1_CHANNEL_4,   ADC1 channel 4 is GPIO4 
    ADC1_CHANNEL_5,   ADC1 channel 5 is GPIO5 
    ADC1_CHANNEL_6,   ADC1 channel 6 is GPIO6 

2) el nivel de atenuacion. normalmente e suele escoger el nivel mas alto
de atenuacion

    +----------+-------------+-----------------+
     |          | attenuation | suggested range |
     |    SoC   |     (dB)    |      (mV)       |
     +==========+=============+=================+
     |          |       0     |    100 ~  950   |
     |          +-------------+-----------------+
     |          |       2.5   |    100 ~ 1250   |
     |   ESP32  +-------------+-----------------+
     |          |       6     |    150 ~ 1750   |
     |          +-------------+-----------------+
     |          |      11     |    150 ~ 2450   |  <-------
     +----------+-------------+-----------------+
     |          |       0     |      0 ~  750   |
     |          +-------------+-----------------+
     |          |       2.5   |      0 ~ 1050   |
     | ESP32-S2 +-------------+-----------------+
     |          |       6     |      0 ~ 1300   |
     |          +-------------+-----------------+
     |          |      11     |      0 ~ 2500   |
     +----------+-------------+-----------------+

*/
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
/*
Se utiza el metodo adc1_config_width() para poder establecer el numero
de bits del adc. para el ESP32 se puede egir entre 8 y 12 bits.
*/
    adc1_config_width(ADC_WIDTH_BIT_12);
    return ESP_OK;
}
