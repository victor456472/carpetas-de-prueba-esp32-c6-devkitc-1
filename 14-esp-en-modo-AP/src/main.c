#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

#define AP_SSID "RED ESP32 VICTOR"
#define AP_PASSWORD "12345678"
#define MAX_STA_CONN 4

static const char *TAG = "WIFI_AP";
static bool ap_configured = false; // Variable para indicar si el modo AP fue configurado exitosamente

void wifi_init_softap(void) {
    ESP_LOGI(TAG, "Initializing Wi-Fi in AP mode...");

    // Inicializar netif
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_netif_init failed: %d", err);
        return;
    }
    ESP_LOGI(TAG, "Netif initialized");

    // Crear bucle de eventos
    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Event loop creation failed: %d", err);
        return;
    }
    ESP_LOGI(TAG, "Event loop created");

    // Crear interfaz AP
    esp_netif_t *netif = esp_netif_create_default_wifi_ap();
    if (netif == NULL) {
        ESP_LOGE(TAG, "Failed to create netif for AP");
        return;
    }
    ESP_LOGI(TAG, "Default Wi-Fi AP created");

    // Configuración inicial de Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi init failed: %d", err);
        return;
    }
    ESP_LOGI(TAG, "Wi-Fi driver initialized");

    // Configuración de AP
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .password = AP_PASSWORD,
            .ssid_len = strlen(AP_SSID),
            .channel = 1,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };
    if (strlen(AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set AP mode: %d", err);
        return;
    }
    ESP_LOGI(TAG, "Wi-Fi set to AP mode");

    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure AP: %d", err);
        return;
    }
    ESP_LOGI(TAG, "Wi-Fi AP configured");

    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Wi-Fi: %d", err);
        return;
    }
    ESP_LOGI(TAG, "AP started with SSID: %s, Password: %s", AP_SSID, AP_PASSWORD);

    // Configuración exitosa
    ap_configured = true;
}

void app_main(void) {
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_LOGI(TAG, "NVS initialized");

    // Inicializar Wi-Fi en modo AP
    wifi_init_softap();

    // Bucle para imprimir el estado
    while (1) {
        if (ap_configured) {
            printf("Modo AP\n");
        } else {
            printf("Configuracion fallida\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Pausa de 1 segundo
    }
}