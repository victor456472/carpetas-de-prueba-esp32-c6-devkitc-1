#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define BUTTON_GPIO GPIO_NUM_18

#define TAG "ADC"
#define SAMPLE_PERIOD_MS 100
#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_5
#define ADC_ATTENUATION ADC_ATTEN_DB_6
#define ADC_BITWIDTH ADC_BITWIDTH_12

static TimerHandle_t xTimerADC = NULL;
static adc_oneshot_unit_handle_t adc_handle = NULL;

volatile bool timer_toggle_request = false; // Indica si el botón fue presionado
volatile bool timer_running = false;

esp_err_t set_timer_adc(void);
esp_err_t start_timer_adc(void);
esp_err_t stop_timer_adc(void);
static void set_adc(void);
void button_task(void *arg);

int adc_val = 0; // Variable que almacena la lectura del ADC

// ✅ ISR modificada: No llama directamente a FreeRTOS
static void IRAM_ATTR button_isr_handler(void *arg)
{
    static uint32_t last_press_time = 0;
    uint32_t current_time = xTaskGetTickCountFromISR();
    
    if ((current_time - last_press_time) > pdMS_TO_TICKS(200)) // Evita rebotes
    {
        last_press_time = current_time;
        timer_toggle_request = true; // Marca que el botón fue presionado
    }
}

// ✅ Tarea para manejar el botón sin afectar la ISR
void button_task(void *arg)
{
    while (true)
    {
        if (timer_toggle_request)
        {
            timer_toggle_request = false;

            if (timer_running)
            {
                stop_timer_adc();
            }
            else
            {
                start_timer_adc();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // Evita consumo excesivo de CPU
    }
}

// Configuración del botón con interrupción
void setup_button(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE // Interrupción en flanco de subida
    };

    gpio_config(&io_conf);

    // Registrar el manejador de interrupción
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    // Crear una tarea para manejar la acción del botón fuera de la ISR
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}

// Función de callback del Timer
void vTimerCallback(TimerHandle_t pxTimer)
{
    adc_val = 0;
    esp_err_t ret = adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_val);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Lectura del sensor: %d", adc_val);
    }
    else
    {
        ESP_LOGE(TAG, "Error al leer el ADC: %s", esp_err_to_name(ret));
    }
}

// Función principal
void app_main(void)
{
    set_adc();
    set_timer_adc();
    setup_button();

    stop_timer_adc(); // Iniciar con el timer apagado
}

// Creación del timer
esp_err_t set_timer_adc(void)
{
    xTimerADC = xTimerCreate(
        "Timer",
        (pdMS_TO_TICKS(SAMPLE_PERIOD_MS)),
        pdTRUE,
        (void *)0,
        vTimerCallback);

    if (xTimerADC == NULL)
    {
        ESP_LOGE(TAG, "Error creando el timer.");
        return ESP_FAIL;
    }

    return ESP_OK;
}

// ✅ Usa xTimerStartFromISR en lugar de xTimerStart
esp_err_t start_timer_adc()
{
    if (xTimerADC == NULL)
    {
        ESP_LOGE(TAG, "El timer no ha sido inicializado.");
        return ESP_FAIL;
    }

    if (!timer_running) // Evita reiniciar si ya está corriendo
    {
        if (xTimerStart(xTimerADC, 0) != pdPASS)
        {
            ESP_LOGE(TAG, "Error al iniciar el timer.");
            return ESP_FAIL;
        }
        timer_running = true;
        ESP_LOGI(TAG, "Timer iniciado.");
    }
    return ESP_OK;
}

// ✅ Usa xTimerStopFromISR en lugar de xTimerStop
esp_err_t stop_timer_adc()
{
    if (xTimerADC == NULL)
    {
        ESP_LOGE(TAG, "El timer no ha sido inicializado.");
        return ESP_FAIL;
    }

    if (timer_running) // Evita detener si ya está apagado
    {
        if (xTimerStop(xTimerADC, 0) != pdPASS)
        {
            ESP_LOGE(TAG, "Error al detener el timer.");
            return ESP_FAIL;
        }
        timer_running = false;
        ESP_LOGI(TAG, "Timer detenido.");
    }
    return ESP_OK;
}

// Configuración del ADC
static void set_adc(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTENUATION};

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));
}
