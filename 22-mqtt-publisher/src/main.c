#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#define WIFI_SSID      "JOptionPane"
#define WIFI_PASSWORD  "System.out.println"
#define MQTT_BROKER_HOST "192.168.51.97"  // Cambia a la IP de tu PC si Mosquitto está corriendo localmente
#define MQTT_BROKER_PORT 1883

static const char *TAG = "MQTT_EXAMPLE";
static esp_mqtt_client_handle_t client = NULL;

static void mqtt_publish_task(void *pvParameters) {
    while (1) {
        const char *mensaje = "ESP32: 15 ppm";
        int msg_id = esp_mqtt_client_publish(client, "sensores/gas", mensaje, 0, 1, 0);
        ESP_LOGI(TAG, "Mensaje enviado (ID=%d): %s", msg_id, mensaje);
        vTaskDelay(pdMS_TO_TICKS(500));  // Espera 5 segundos
    }
}

static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Conectado al broker MQTT");
            xTaskCreate(mqtt_publish_task, "mqtt_pub_task", 4096, NULL, 5, NULL);
            break;
        default:
            ESP_LOGI(TAG, "Evento MQTT recibido: %d", event->event_id);
            break;
    }
}


static void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.hostname = MQTT_BROKER_HOST,
        .broker.address.port = MQTT_BROKER_PORT,
        .broker.address.transport = MQTT_TRANSPORT_OVER_TCP,
        .credentials.username = "sensor1",
        .credentials.authentication.password = "prueba1234"
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, NULL);
    esp_mqtt_client_start(client);
}

static void wifi_init(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(10000));
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init();
    mqtt_app_start();
}
