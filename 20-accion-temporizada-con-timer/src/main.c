#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"  // Librería para medir tiempo en microsegundos

#define BUTTON_GPIO 18       // Pin GPIO donde está conectado el botón
#define HOLD_TIME_MS 10000   // Tiempo en milisegundos para considerar que el botón está presionado (10 segundos)

// Prototipo de funciones
void init_gpio();
void press_button(void *param);

void app_main(void) {
    // Inicializar GPIO
    init_gpio();

    // Crear la tarea para manejar el botón
    xTaskCreate(press_button, "press_button", 2048, NULL, 10, NULL);
}

// Función para inicializar el GPIO
void init_gpio() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO), // Seleccionar el pin
        .mode = GPIO_MODE_INPUT,               // Configurar como entrada
        .pull_up_en = GPIO_PULLUP_DISABLE,     // Deshabilitar pull-up
        .pull_down_en = GPIO_PULLDOWN_ENABLE,  // Habilitar pull-down
        .intr_type = GPIO_INTR_DISABLE         // Sin interrupciones
    };
    gpio_config(&io_conf);
}

// Tarea que monitorea el estado del botón
void press_button(void *param) {
    uint64_t press_start_time = 0;  // Tiempo en que el botón fue presionado
    bool button_held = false;       // Estado del botón (si fue mantenido por más de 10 segundos)

    while (1) {
        // Leer el estado del botón
        int button_state = gpio_get_level(BUTTON_GPIO);

        if (button_state == 1) {  // Botón presionado
            if (press_start_time == 0) {
                // Registrar el momento en que se presionó el botón
                press_start_time = esp_timer_get_time() / 1000; // Tiempo actual en ms
            } else {
                // Calcular cuánto tiempo ha estado presionado
                uint64_t elapsed_time = (esp_timer_get_time() / 1000) - press_start_time;
                if (elapsed_time >= HOLD_TIME_MS && !button_held) {
                    button_held = true; // Marcamos que el botón fue mantenido
                    printf("Botón mantenido por más de 10 segundos\n");
                }
            }
        } else {  // Botón no presionado
            press_start_time = 0; // Reiniciar el tiempo de inicio
            button_held = false;  // Reiniciar el estado del botón mantenido
        }

        vTaskDelay(10 / portTICK_PERIOD_MS); // Pequeña espera para reducir consumo de CPU
    }
}
