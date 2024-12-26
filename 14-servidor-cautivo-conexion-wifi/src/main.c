#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

static const char *TAG = "CAPTIVE_PORTAL";

// Nombres/contraseñas de ejemplo
#define AP_SSID "MyDeviceConfig"
#define AP_PASS "12345678"   // mínimo 8 caracteres si se desea WPA2

// Función para iniciar el modo AP
void start_softap(void)
{
    // Inicializa la pila de red
    esp_netif_init();
    esp_event_loop_create_default();

    // Crea la interfaz Wi-Fi en modo AP
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Configura los parámetros del AP
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .password = AP_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "AP iniciado: SSID:%s password:%s", AP_SSID, AP_PASS);
}

// Manejador de la página principal
static esp_err_t root_get_handler(httpd_req_t *req)
{
    // Página HTML para formulario
    const char* resp_str =
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>Config Wi-Fi</title></head>"
        "<body>"
        "<h2>Configurar Wi-Fi</h2>"
        "<form action=\"/setWifi\" method=\"post\">"
        "  SSID: <input type=\"text\" name=\"ssid\"><br>"
        "  Password: <input type=\"password\" name=\"pass\"><br>"
        "  <input type=\"submit\" value=\"Guardar\">"
        "</form>"
        "</body>"
        "</html>";

    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Manejador para procesar formulario
static esp_err_t set_wifi_post_handler(httpd_req_t *req)
{
    // 1. Leer el cuerpo de la petición (los datos del formulario)
    char content[100];
    int total_len = req->content_len;
    int received = 0;

    if (total_len >= sizeof(content)) {
        // Exceso de datos (simplificado)
        return ESP_FAIL;
    }

    while (received < total_len) {
        int ret = httpd_req_recv(req, content + received, total_len - received);
        if (ret <= 0) {
            return ESP_FAIL;
        }
        received += ret;
    }
    content[received] = '\0';

    // content podría ser algo como: "ssid=MiSSID&pass=MiPassword"
    // 2. Extraer el ssid y pass (aquí se hace parse manual o se usa una librería).
    char ssid[32] = {0}, pass[64] = {0};
    // Ejemplo muy simple (busca patrones):
    sscanf(content, "ssid=%[^&]&pass=%s", ssid, pass); 
    // Ojo con decodificar URL si contiene caracteres especiales.

    // 3. Guardar en NVS (ejemplo simplificado)
    nvs_handle_t nvs;
    nvs_open("wifi_store", NVS_READWRITE, &nvs);
    nvs_set_str(nvs, "ssid", ssid);
    nvs_set_str(nvs, "pass", pass);
    nvs_commit(nvs);
    nvs_close(nvs);

    ESP_LOGI(TAG, "Guardado en NVS - SSID: %s, PASS: %s", ssid, pass);

    // 4. Enviar respuesta de confirmación
    const char* resp_str = "Datos guardados. El dispositivo intentará conectarse...";
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    // 5. (Opcional) Iniciar reconexión en modo Station
    //    Podrías postear un evento a la app para que cambie a STA.

    return ESP_OK;
}

// Registramos URIs en el servidor
void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        // Pagina raíz: GET -> root_get_handler
        httpd_uri_t root_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root_uri);

        // Ruta /setWifi: POST -> set_wifi_post_handler
        httpd_uri_t setWifi_uri = {
            .uri = "/setWifi",
            .method = HTTP_POST,
            .handler = set_wifi_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &setWifi_uri);
    }
}

// app_main: inicia NVS, arranca AP, inicia servidor web.
void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret==ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Iniciar AP
    start_softap();

    // Iniciar servidor web
    start_webserver();

    // Quedas en modo AP con la web de configuración
    // El usuario ingresa SSID/PASS, se guarda en NVS
    // y luego en tu lógica podrías cambiar a modo STA.
}