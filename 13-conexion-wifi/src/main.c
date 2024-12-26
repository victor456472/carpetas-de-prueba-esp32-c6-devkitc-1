#include <stdio.h>
#include <string.h>

//librerias de FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//librerias de Espressif
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_err.h"

// Se definen las credenciales de conexion a la red:
#define EXAMPLE_ESP_WIFI_SSID "JOptionPane"
#define EXAMPLE_ESP_WIFI_PASS "System.out.println"

//etiqueta para los logs
static const char *TAG = "WIFI_STATION";

/**
 * event_handler
 *
 * Esta función se encarga de manejar los eventos de Wi-Fi e IP que llegan al
 * sistema. El ESP-IDF utiliza un sistema de "eventos" para notificar cambios
 * en el estado de la conexión Wi-Fi, la obtención de la IP, desconexiones, etc.
 */

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    // 1. WIFI_EVENT_STA_START: Se dispara cuando el Wi-Fi Station se inicia.
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // Iniciamos la conexión a la red. 
        // "esp_wifi_connect()" usa las credenciales definidas en wifi_config.
        esp_wifi_connect();

    // 2. WIFI_EVENT_STA_DISCONNECTED: Se dispara cuando la estación se desconecta
    //    del AP (Access Point) o falla la conexión.
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {  
        ESP_LOGI(TAG, "Conexión perdida. Reintentando...");
        // Intentamos reconectarnos automáticamente.
        esp_wifi_connect();
    // 3. IP_EVENT_STA_GOT_IP: Se dispara cuando finalmente se obtiene IP del router.
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // "event_data" contiene información acerca de la IP obtenida.
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado, IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

/**
 * wifi_init_sta
 *
 * Función que configura e inicia el modo Station (STA) de Wi-Fi.
 */
void wifi_init_sta(void) {
    // 1. Inicializar la interfaz de red (esp_netif_init).
    //    Esto prepara la parte de TCP/IP, sockets, etc.
    ESP_ERROR_CHECK(esp_netif_init());
    // 2. Crear el bucle de eventos por defecto.
    //    Este bucle maneja los eventos de Wi-Fi, IP, etc.
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // 3. Crear la interfaz Wi-Fi Station por defecto.
    //    Esto asocia la interfaz de red del chip al modo Station.
    esp_netif_create_default_wifi_sta();

    // 4. Inicializar la configuración por defecto del Wi-Fi.
    //    El macro "WIFI_INIT_CONFIG_DEFAULT()" define parámetros internos
    //    como buffers, colas, etc.
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 5. Registrar la función que manejará los eventos de Wi-Fi e IP.
    //    - WIFI_EVENT: cualquier evento relacionado con Wi-Fi (iniciar, desconectar, etc.)
    //    - IP_EVENT: cualquier evento relacionado con la obtención de IP.
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &event_handler,
        NULL
    ));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &event_handler,
        NULL
    ));

    // 6. Crear la estructura de configuración para Wi-Fi Station.
    //    - Se definen el SSID y la contraseña.
    //    - Podemos asignar canal, tipo de seguridad, etc. si fuera necesario.
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            // Otros parámetros opcionales
        },
    };

    // 7. Fijar el modo Wi-Fi a Station (WIFI_MODE_STA).
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // 8. Aplicar la configuración con las credenciales al interfaz Station.
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // 9. Iniciar el Wi-Fi con los parámetros ya cargados.
    ESP_ERROR_CHECK(esp_wifi_start());

    // Log informativo
    ESP_LOGI(TAG, "wifi_init_sta terminado.");
}

void app_main(void) {
    // 1. Inicializar NVS (Non-Volatile Storage).
    //    Se usa para guardar datos en la flash, como credenciales y otra info.
    esp_err_t ret = nvs_flash_init();

    // En caso de que la partición de NVS esté llena o sea de otra versión,
    // se borra y se vuelve a inicializar para prevenir errores.
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Imprimimos un mensaje de log para indicar que empezaremos la configuración Wi-Fi
    ESP_LOGI(TAG, "Iniciando Wi-Fi Station...");

    // 3. Llamamos a la función que configura e inicia la Wi-Fi Station
    wifi_init_sta();
}