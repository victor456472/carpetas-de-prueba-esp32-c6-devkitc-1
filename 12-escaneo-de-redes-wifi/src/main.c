/*
Esta practica consiste en escanear los access point de la red wifi.

el desarrollo del codigo se basó en el tutorial que se muestra a continuacion:

https://youtube.com/playlist?list=PLmQ7GYcMY-2JV7afZ4hiekn8D6rRIgYfj&si=cQHAoQEpCTzz2vWw

video #11

sin embargo, para el caso del esp32-c6-DevKitC-1 habia que actualizar algunos metodos
que quedaron obsoletos desde la fecha de la publicación. algunos de ellos son los
pertenencientes a esp_event_loop.h que ahora migraron a esp_event.h

Es necesario documentar el codigo desarrollado para su commprensión
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

void wifi_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init()); // Inicialización de la interfaz de red
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta(); // Crea el default para modo estación

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
}

void wifi_scan(void) {
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true // Escanea incluso redes ocultas
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true)); // Iniciar el escaneo

    uint16_t ap_num = 20; // Máximo número de puntos de acceso a listar
    wifi_ap_record_t ap_records[20];
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records)); // Obtener resultados

    ESP_LOGI("WiFi Scan", "Número de AP encontrados: %d", ap_num);
    ESP_LOGI("WiFi Scan", "SSID | CHANNEL | RSSI | MAC");

    for (int i = 0; i < ap_num; i++) {
        ESP_LOGI("WiFi Scan", "%32s | %7d | %4d | %02x:%02x:%02x:%02x:%02x:%02x",
                 ap_records[i].ssid,
                 ap_records[i].primary,
                 ap_records[i].rssi,
                 ap_records[i].bssid[0], ap_records[i].bssid[1],
                 ap_records[i].bssid[2], ap_records[i].bssid[3],
                 ap_records[i].bssid[4], ap_records[i].bssid[5]);
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // Configura en modo estación
    ESP_ERROR_CHECK(esp_wifi_start());

    while (true) {
        ESP_LOGI("WiFi Scan", "Iniciando escaneo de redes WiFi...");
        wifi_scan();
        vTaskDelay(pdMS_TO_TICKS(10000)); // Espera 10 segundos entre escaneos
    }
}
