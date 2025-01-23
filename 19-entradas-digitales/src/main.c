#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BUTTON_GPIO 18  // Pin GPIO donde está conectado el botón

void app_main(void) {
    // Configurar el pin del botón como entrada sin pull-up/pull-down interno
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO), // Seleccionar el pin
        .mode = GPIO_MODE_INPUT,               // Configurar como entrada
        .pull_up_en = GPIO_PULLUP_DISABLE,     // Deshabilitar pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Deshabilitar pull-down
        .intr_type = GPIO_INTR_DISABLE         // Sin interrupciones
    };
    gpio_config(&io_conf);

    while (1) {
        // Leer el estado del botón
        int button_state = gpio_get_level(BUTTON_GPIO);
        if (button_state == 1) {
            printf("Botón presionado\n");
        } else {
            printf("Botón no presionado\n");
        }
        vTaskDelay(500 / portTICK_PERIOD_MS); // Esperar 500 ms
    }
}
