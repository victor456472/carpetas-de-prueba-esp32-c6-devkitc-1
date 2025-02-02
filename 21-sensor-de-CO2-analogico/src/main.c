#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define TAG "ADC"

#define SAMPLE_PERIOD_MS 100

#define ADC_UNIT ADC_UNIT_1
/*
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
*/
#define ADC_CHANNEL ADC_CHANNEL_5
/*
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
#define ADC_ATTENUATION ADC_ATTEN_DB_6
#define ADC_BITWIDTH ADC_BITWIDTH_12

static TimerHandle_t xTimerADC=NULL;
static adc_oneshot_unit_handle_t adc_handle = NULL;

esp_err_t set_timer(void);
static void set_adc(void); //funcion para configurar el ADC 

/*ADC*/

int adc_val = 0; // variable que alacena la lectura del ADC


void vTimerCallback(TimerHandle_t pxTimer)
{
    adc_val = 0;
    esp_err_t ret = adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_val);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "lectura del sensor: %d", adc_val);
    } else {
        ESP_LOGE(TAG, "Error al leer el ADC: %s", esp_err_to_name(ret));
    }
}

void app_main(void)
{
    set_adc();
    set_timer();
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

esp_err_t set_timer(void)
{
    xTimerADC = xTimerCreate(
        "Timer",                   
        (pdMS_TO_TICKS(SAMPLE_PERIOD_MS)), 
        pdTRUE,                   
        (void *)0,           
        vTimerCallback             
    );

    if (xTimerADC == NULL) {
        ESP_LOGE(TAG, "Error creando el timer.");
        return ESP_FAIL;
    }

    // Iniciar el timer
    if (xTimerStart(xTimerADC, 0) != pdPASS) {
        ESP_LOGE(TAG, "Error iniciando el timer.");
        return ESP_FAIL;
    }

    return ESP_OK;
}
static void set_adc(void)
{
    
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTENUATION
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));
}
