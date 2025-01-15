#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

#define AP_SSID "RED ESP32 VICTOR"
#define AP_PASSWORD "ChispitA15973"
#define MAX_STA_CONN 4

static const char *TAG = "WIFI_AP";

// Página HTML del formulario
static const char *html_form = 
    "<!DOCTYPE html>"
    "<html>"
    "<head><title>Formulario ESP32</title></head>"
    "<body>"
    "<h1>Formulario</h1>"
    "<form action=\"/submit\" method=\"GET\">"
    "  <label for=\"dato\">Ingrese un dato:</label>"
    "  <input type=\"text\" id=\"dato\" name=\"dato\">"
    "  <input type=\"submit\" value=\"Enviar\">"
    "</form>"
    "</body>"
    "</html>";

// Manejar la raíz (sirve el formulario)
esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_form, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Manejar envío del formulario
esp_err_t submit_handler(httpd_req_t *req) {
    char param[100];
    if (httpd_req_get_url_query_str(req, param, sizeof(param)) == ESP_OK) {
        char value[50];
        if (httpd_query_key_value(param, "dato", value, sizeof(value)) == ESP_OK) {
            ESP_LOGI(TAG, "Dato recibido: %s", value);
            httpd_resp_sendstr(req, "Dato recibido correctamente.");
        } else {
            httpd_resp_sendstr(req, "Error al procesar el dato.");
        }
    } else {
        httpd_resp_sendstr(req, "No se recibieron parámetros.");
    }
    return ESP_OK;
}

// Manejar redirección genérica
esp_err_t not_found_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// Configurar servidor HTTP
void start_http_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root);

        httpd_uri_t submit = {
            .uri = "/submit",
            .method = HTTP_GET,
            .handler = submit_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &submit);

        httpd_uri_t redirect = {
            .uri = "/*",
            .method = HTTP_GET,
            .handler = not_found_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &redirect);
    } else {
        ESP_LOGE(TAG, "Error al iniciar el servidor HTTP");
    }
}

// Inicializar Wi-Fi en modo AP
void wifi_init_softap(void) {
    ESP_LOGI(TAG, "Inicializando Wi-Fi en modo AP...");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

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

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Modo AP iniciado con SSID: %s, Contraseña: %s", AP_SSID, AP_PASSWORD);
}

void app_main(void) {
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Inicializar Wi-Fi en modo AP
    wifi_init_softap();

    // Iniciar servidor HTTP
    start_http_server();
}
