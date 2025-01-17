#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"

#define AP_SSID "RED ESP32 VICTOR"
#define AP_PASSWORD "12345678"
#define MAX_STA_CONN 4

static const char *TAG = "WIFI_AP";

/**
 * @brief Esta funcion inicializa el sistema de archivos 
 * SPIFFS (SPI Flash File System) en un ESP32 utilizando
 * la API de ESP-IDF. esta no toma ningun parametro ni devuelva
 * ningun valor.
 */
void init_spiffs(void) {

    /**
     * a continuacion se escribe la estructura 
     * esp_vfs_spiffs_conf_t la cual es proporcionada 
     * por ESP-IDF para configurar la inicializacion 
     * del sistema de archivos  SPIFFS.
     * 
     * esta estructura tiene los siguientes parametros:
     * 
     * @param[in] base_path define el punto de montaje del
     * sistema de archivos. en este caso, todos los archivos
     * SPIFFS serán accesibles bajo el prefijo "/spiffs"
     * 
     * @param[in] partition_label Especifica el nombre de 
     * la particion SPIFFS en la tabla de particiones. en 
     * este caso se establece en NULL lo que significa que
     * se usará la partición predeterminada etiquetada para 
     * SPIFFS.
     * 
     * @param[in] max_files Numero máximo de archivos quer
     * se pueden abrir simultaneamente. en este caso se
     * configura en 5 archivos.
     * 
     * @param[in] format_if_mount_failed si es True, el 
     * sistema intentará formatear la partición si falla 
     * al montarla.
     */
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    /**
     * esta funcion monta el sistema de archivos SPIFFS
     * basado en la configuración proporcionada en conf.
     * Devuelve un código de error (esp_err_t), que indica 
     * si la operación fue exitosa o no.
     */

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    /**
     * Finalmente se evalua la respuesta de la funcion
     * eps_vfs_spiffs_register. si este retorna algo
     * distinto a ESP_OK se imprime el error en el monitor
     * serial, indicando que hubo un fallo al inicializar 
     * SPIFFS.
     * 
     * en el caso que retorne ESP_OK, la operacion habrá sido
     * exitosa y se imprimirá el mensaje "SPIFFS inicializado 
     * correctamente".
     */
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar SPIFFS (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS inicializado correctamente");
    }
}

/* 
la siguiente funcion es un manejador para solucitudes HTTP que se 
utiliza para servir la pagina principal del servidor web.

tiene como parametro el argumento httpd_req_t *req el cual es un
puntero a la estructura de la solicitud HTTP que contiene informacion
sobre la solicitud entrante como la ruta solicitada, los encabezados
y más.
*/ 
esp_err_t root_handler(httpd_req_t *req) {
    FILE *file = fopen("/spiffs/index.html", "r");
    if (!file) {
        ESP_LOGE(TAG, "No se pudo abrir el archivo HTML");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    char buffer[1024];
    size_t read_bytes;
    httpd_resp_set_type(req, "text/html");

    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        httpd_resp_send_chunk(req, buffer, read_bytes);
    }
    httpd_resp_send_chunk(req, NULL, 0); // Fin de la respuesta
    fclose(file);
    return ESP_OK;
    return ESP_OK;
}

/*
la siguiente funcion es un manejador para procesar los datos enviados
desde un formulario web y responder al cliente con un mensaje
apropiado
*/ 
esp_err_t submit_handler(httpd_req_t *req) {
    /*
    el siguiente es un buffer donde se almacenará la cadena de 
    parametros de consulta extraida de la URL. tiene un tamaño
    de 100 bytes lo cual define  la longitud maxima de la cadena
    que puede manejar.
    */
    char param[100];
    /*
    el siguiente condicional extrae la cadena de parametros de
    consulta de la URL.

    se puede explicar el condiconal de la siguiente forma:
    httpd_req_get_url_query_str permite recibir el GET request y 
    devuelve ESP_OK en caso de ser exitoso. si ese es el caso el
    condicional entrara en estado "verdadero" por lo cual se creará
    un procedimiento para rescatar el dato del GET request.

    en caso de que no haya sido posible extraer la cadena de
    parametros de la URL se imprime en monitor serial un 
    ESP_LOG con el mensaje "No se recibieron parametros."

    informacion tecnica adicional:

    La función httpd_req_get_url_query_str extrae la cadena de 
    parámetros de consulta de la URL (es decir, todo lo que viene 
    después del ? en la URL, por ejemplo, dato=valor).

    parametros:

    req: la solicitud HTTP que se está manejando
    param: el buffer donde se guardarán los parametros
    sizeof(param): El tamaño del buffer.
    */
    if (httpd_req_get_url_query_str(req, param, sizeof(param)) == ESP_OK) {
        /*
        El siguiente es un buffer que almacenará el valor de un
        parametro especifico, en este caso, el asociado con la clave
        "dato". este tiene un tamaño de 50 bytes.
        */
        char value[50];
        /*
        el siguiente condicional permite extraer el valor del parametro
        "dato". este emmpieza evaluando si es posible hacer el query
        mediante la funcion "httpd_query_key_value()" la cual retorna
        ESP_OK si es exitoso y a su vez rescata el dato en el buffer.

        una vez se compruebe el query, imprime el dato con un ESP_LOG
        y se envia como respuesta a la pagina Web el mensaje "dato
        recibido correctamente."

        en caso de que el query no haya sido posible se envia como 
        respuesta a la pagina Web el mensaje "Error al
        procesar el dato."
        */
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

/*
El siguiente es un manejador para solicitudes HTTP que no coinciden
con ninguna ruta especifica configurada en el servidor. Es un 
redireccionador que responde con un código de estado HTTP 302
not found y redirige al cliente a la pagina principal (/).

informacion tecnica:

parametros:
httpd_req_t *req: Solicitud HTTP recibida por el servidor.
*/  
esp_err_t not_found_handler(httpd_req_t *req) {
    /*
    la siguiente funcion establece el codigo de estado HTTP de la
    respuesta:
    302 found -> es un codigo de redireccion que indica al cliente
    que el recurso solicitado ha sido movido temporalmente a otra
    ubicacion.

    se usa comunmente para redirigir a otra URL.
    */
    httpd_resp_set_status(req, "302 Found");
    /*
    la siguiente función agrega el encabezado HTTP "location" a la 
    respuesta.

    informacion tecnica:

    el encabezado Location especifica la nueva URL a la  que debe 
    redirigirse el cliente (en este caso a la pagina principal del
    servidor web "/")
    */
    httpd_resp_set_hdr(req, "Location", "/");
    /*
    La siguiente funcion envia la respuesta HTTP al cliente. en este
    caso, el cuerpo de la respuesta es NULL (vacío) y la longitud es 0.
    esto significa que la respuesta no incluye contenido, solo el
    encabezado HTTP.
    */
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// Iniciar servidor HTTP
/**
 * @brief Iniciar servidor HTTP
 * 
 * esta funcion se encarga de inicializar y configurar un servidor
 * HTTP en la ESP32. Es el punto de partida para que el servidor maneje
 * solicitudes HTTP de los clientes y responda con contenido o 
 * redirecciones.
 * 
 * @return Nada
 */
void start_http_server(void) {
    /**
     * @brief la siguiente estrucutra permite configurar el servidor HTTP con 
     * valores por defecto usando la macro HTTPD_DEFAULT_CONFIG().
     * 
     * algunos parametros predeterminados incluyen:
     * @param[in] Puerto 80 (HTTP estándar)
     * @param[in] max_open_sockets Número maximo de conexiones simultaneas
     * @param[in] stack_size tamaño del buffer para solicitudes
     */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    /**
     * @brief esta otra estructura declara un manejador para el servidor 
     * HTTP, que se usará para registrar rutas y controladores.
     */
    httpd_handle_t server = NULL;

    /**
     * @brief inicia el servidor HTTP con la configuracion especificada
     * (config) y asigna el manejador al puntero (server)
     */
    if (httpd_start(&server, &config) == ESP_OK) {
        /**
         * @brief Configura un manejador para a ruta raiz que se activa
         * cuando un cliente envía una solicitud GET a la URL (/).
         * 
         * los campos que maneja la estructura son:
         * 
         * @param[in] uri especifica la ruta raiz del servidor. para este caso es "/"
         * @param[in] method tipo de solicitud a la cual responde el manejador. en este caso es GET.
         * @param[in] handler asocia el manejador de solicitudes que para este caso es la funcion roor_handler()
         * @param[in] user_ctx es un campo opcional para pasar datos personalizados al manejador. en este caso no se usa.
         */
        httpd_uri_t root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_handler,
            .user_ctx = NULL
        };
        /**
         * la siguiente funcion registra el manejador en el servidor,
         * asociandolo con la ruta especificada.
         */
        httpd_register_uri_handler(server, &root);

        /**
         * Configura y registra un manejador para la ruta /submit la cual
         * se usa cuando el cliente presiona el boton "Enviar".
         */
        httpd_uri_t submit = {
            .uri = "/submit",
            .method = HTTP_GET,
            .handler = submit_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &submit);
        /**
         * Configura y registra un manejador genérico para todas las rutas
         * no definidas explicitamente.
         */
        httpd_uri_t redirect = {
            .uri = "/*",
            .method = HTTP_GET,
            .handler = not_found_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &redirect);
    } else {
        /**
         * en caso de que el servidor no se haya iniciado se imprime el
         * error en el monitor serie.
         */
        ESP_LOGE(TAG, "Error al iniciar el servidor HTTP");
    }
}

// Inicializar Wi-Fi en modo AP
/**
 * @brief Inicializa el Wi-Fi en modo Access Point (AP).
 *
 * Esta función configura y inicia el Wi-Fi en modo AP, utilizando las constantes
 * AP_SSID y AP_PASSWORD para establecer el nombre de la red y la contraseña.
 * También establece el canal, el número máximo de conexiones simultáneas y el
 * modo de autenticación según la longitud de la contraseña.
 *
 * @note Esta función utiliza las funciones y estructuras de ESP-IDF para la
 * gestión del Wi-Fi.
 *
 * @return Nada.
 */
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
    // Inicializar SPIFFS
    init_spiffs();

    // Inicializar Wi-Fi en modo AP
    wifi_init_softap();

    // Iniciar servidor HTTP
    start_http_server();
}
