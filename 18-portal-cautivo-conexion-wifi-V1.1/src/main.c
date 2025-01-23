#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "led_strip.h"
#include "nvs.h"

#define AP_SSID "RED ESP32 VICTOR"
#define AP_PASSWORD "12345678"
#define MAX_STA_CONN 4

#define GPIO_WS2812B 8
#define MAX_LEDS 1

#define COLOR_CYAN 0,255,255
#define COLOR_LIGHT_GREEN 0,255,0
#define COLOR_RED 255,0,0
#define COLOR_YELLOW 255,255,0

#define NVS_NAMESPACE "wifi_config"
#define NVS_KEY_MODE "mode"
#define NVS_KEY_SSID "ssid"
#define NVS_KEY_PASSWORD "password"

static const char *TAG = "WIFI_AP";
static led_strip_t *led_strip=NULL;

bool borrar_memoria = false;

// Función para guardar el estado en NVS
void save_wifi_config_to_nvs(wifi_mode_t mode, const char *ssid, const char *password) {
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle));

    // Guardar estado
    ESP_ERROR_CHECK(nvs_set_u8(nvs_handle, NVS_KEY_MODE, mode));

    // Guardar SSID y contraseña si el modo es STA
    if (mode == WIFI_MODE_STA && ssid && password) {
        ESP_ERROR_CHECK(nvs_set_str(nvs_handle, NVS_KEY_SSID, ssid));
        ESP_ERROR_CHECK(nvs_set_str(nvs_handle, NVS_KEY_PASSWORD, password));
    }

    // Confirmar cambios
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    nvs_close(nvs_handle);
}

// Función para leer el estado desde NVS
wifi_mode_t read_wifi_config_from_nvs(char *ssid, size_t ssid_len, char *password, size_t password_len) {
    nvs_handle_t nvs_handle;
    wifi_mode_t mode = WIFI_MODE_AP; // Modo predeterminado

    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) == ESP_OK) {
        // Leer estado
        uint8_t stored_mode;
        if (nvs_get_u8(nvs_handle, NVS_KEY_MODE, &stored_mode) == ESP_OK) {
            mode = (wifi_mode_t)stored_mode;
        }

        // Leer SSID y contraseña si el modo es STA
        if (mode == WIFI_MODE_STA) {
            nvs_get_str(nvs_handle, NVS_KEY_SSID, ssid, &ssid_len);
            nvs_get_str(nvs_handle, NVS_KEY_PASSWORD, password, &password_len);
        }

        nvs_close(nvs_handle);
    }

    return mode;
}

void init_led_strip(void){
    led_strip = led_strip_init(0, GPIO_WS2812B, MAX_LEDS);
    if (!led_strip)
    {
        ESP_LOGE(TAG, "Fallo al inicializar el LED RGB");
        return;
    }

    ESP_ERROR_CHECK(led_strip->clear(led_strip, 100));
}

void set_led_color(uint8_t red, uint8_t green, uint8_t blue){
    if (led_strip)
    {
        ESP_ERROR_CHECK(led_strip->set_pixel(led_strip,0,red,green,blue));
        ESP_ERROR_CHECK(led_strip->refresh(led_strip, 100));
    }
    
}

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
    /**
     * la funcion fopen() abre el archivo especificado en
     * modo lectura ("r"). el archivo debe estar en la particion
     * SPIFFS y disponible en la ruta /spiffs/index.html
     * 
     * file es un puntero al archivo que será leido. si el 
     * archivo no se encuentra o no se puede abrir, fopen
     * devuelve NULL.
     * 
     * la funcion fopen() tiene dos parametros:
     * 
     * @param[in] filename Ruta del archivo que se desea abrir
     * @param[in] mode modo de apertura. este puede tener varios tipos:
     *                 "r" - leer (error si el arhivo no existe)
     *                 "w" - Escribir (crea el archivo si no existe o lo sobrescribe si ya existe)
     *                 "a" - Añadir (escribe al final del archivo)
     *                 "r+" - Leer y escribir
     *                 "w+" - Leer y escribir (sobrescribe el archivo existente)
     *                 "a+" - Leer y escribir (añade al final del archivo)
     * 
     * @note
     * 
     * El tipo de dato FILE en C representa una estructura
     * definida en la biblioteca estándar <stdio.h> que se 
     * utiliza para manejar operaciones de archivos. es una
     * abstraccion que contiene toda la informacion necesaria
     * para interactuar con un archivoen el sistema de
     * archivos.
     * 
     * Es importante aclarar que FILE no es un tipo de datos
     * primitivo como "int" o "char". Es una estructura
     * definida internamente en la biblioteca estandar de C.
     * 
     * la definición de FILE puede variar dependiendo del
     * sistema operativo y la implementación de la biblioteca
     * C, pero tipicamente contiene:
     * 
     * 1) puntero al archivo en el sistema de archivos
     * 
     * 2) buffer interno para manejar datos durante la lectura
     * y escritura
     * 
     * 3) indicador de fin de archivo (EOF)
     * 
     * 4) errores relacionados con la operacion de archivo
     * 
     * FILE se utiliza con las funciones de manejo de archivos
     * como fopen, fclose, fread, fwrite, etc...
     */
    FILE *file = fopen("/spiffs/index.html", "r");

    /**
     * Si el puntero file tiene un valor de NULL significa
     * que la funcion fopen no pudo abrir el archivo por lo
     * cual se evalua la condicion !file ya que si file es 
     * NULL, !file es True y por ende el programa entra a la 
     * condición para imprimir en el monitor serie el mensaje
     * "no se pudo abrir el archivo HTML" ademas de enviar un
     * error 404 al servidor y retornar ESP_FAIL.
     */
    if (!file) {
        ESP_LOGE(TAG, "No se pudo abrir el archivo HTML");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    /**
     * La siguiente función establece el encabezado HTTP 
     * Content-Type de la respuesta como text/html. esto
     * indica al navegador del cliente que el contenido
     * que se esta enviando es una pagina HTML.
     */
    httpd_resp_set_type(req, "text/html");

    /**
     * Se declara un buffer temporal de 1024 bytes para
     * almacenar partes del archivo mientras se lee
     */
    char buffer[1024];

    /**
     * size_t es un tipo de dato sin signo que se utiliza
     * comúnmente para representar tamaños o conteos en C.
     * Es el tipo de retorno de funciones como fread, por 
     * lo cual es el tipo de dato adecuado para almacenar
     * el tamaño del archivo.
     */
    size_t read_bytes;

    /**
     * El siguiente ciclo permite enviar el contenido html
     * al servidor por chunks o "paquetes". para ello primero
     * se usa la funcion fread() la cual lee datos desde el
     * archivo (file) en bloques del tamaño especificado 
     * (1024 bytes) y los guarda en el búfer (buffer).
     * 
     * la funcion fread tiene los siguientes parametros:
     * @param[out] buffer Direccion del buffer donde se 
     * almacenan los datos leidos
     * @param[in] 1 tamaño de cada elemento que se lee 
     * (1 byte en este caso)
     * @param[in] sizeofBuffer maximo número de elementos 
     * a leer (hasta 1024 bytes en este caso)
     * 
     * Al ser invocada con los limites establecidos, la 
     * función devuelve la cantidad real de bytes leidos
     * desde el archivo en cada iteración y guarda el conteo 
     * dentro de read_bytes. si fread encuentra menos bytes 
     * (por ejemplo al final del archivo), el valor será
     * menor que el tamaño del buffer.
     * 
     * Si ocurre un error o se alcanza el final del archiv
     * fread() devuelve 0.
     * 
     * con esto en mente se puede evaluar si read_bytes es
     * mayor a 0. en tal caso se debe enviar el paquete de 
     * datos al cliente. en caso que readbytes sea 0
     * significa que se llegó al final del archivo y se 
     * debe finalizar el ciclo.
     */
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        /**
         * con la funcion httpd_resp_send_chunk() se envia
         * el contenido leido en el fuffer como un fragmento
         * (chunk) al cliente. el tamaño del fragmento es 
         * igual a read_bytes.
         */
        httpd_resp_send_chunk(req, buffer, read_bytes);
    }
    /**
     * A continuacion se envia un fragmento vacio (NULL) de tamaño
     * 0 al cliente. esto indica que ya no se enviarán más datos. 
     * esta acción es obligatoria para finalizar una respuesta
     * con "chunked encoding"
     */
    httpd_resp_send_chunk(req, NULL, 0); 
    /**
     * Finalmente se cierra el archivo abierto por fopen()
     * para liberar recursos del sistema.
     */
    fclose(file);
    return ESP_OK;
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi STA iniciado, conectando...");
        set_led_color(COLOR_YELLOW);
        esp_wifi_connect(); // Iniciar la conexión
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Conexión fallida, intentando reconectar...");
        esp_wifi_connect(); // Reintentar conexión
        set_led_color(COLOR_RED);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Conexión exitosa, dirección IP: " IPSTR, IP2STR(&event->ip_info.ip));
        set_led_color(COLOR_LIGHT_GREEN);
    }
}


void wifi_init_sta(const char *ssid, const char *password) {
    ESP_LOGI(TAG, "Inicializando Wi-Fi en modo STA...");

    // Verificar si el Wi-Fi ya está inicializado
    wifi_mode_t current_mode;
    if (esp_wifi_get_mode(&current_mode) == ESP_OK && current_mode != WIFI_MODE_STA) {
        ESP_ERROR_CHECK(esp_wifi_stop());
    }

    // Crear interfaces STA solo si no existen
    static esp_netif_t *sta_netif = NULL;
    if (!sta_netif) {
        sta_netif = esp_netif_create_default_wifi_sta();
        if (!sta_netif) {
            ESP_LOGE(TAG, "Error al crear la interfaz STA");
            return;
        }
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
        },
    };
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    set_led_color(COLOR_YELLOW);
    ESP_LOGI(TAG, "Conectándose a SSID: %s", ssid);

    // Intentar conexión
    esp_err_t ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        set_led_color(COLOR_RED);
        ESP_LOGE(TAG, "Error al intentar conectarse: %s", esp_err_to_name(ret));
    } else {
        set_led_color(COLOR_LIGHT_GREEN);
        ESP_LOGI(TAG, "Conectado a la red");
    }
}

void url_decode(char *src, char *dest, size_t dest_size) {
    char *end = src + strlen(src);
    char *d = dest;
    char *d_end = dest + dest_size - 1; // Deja espacio para el terminador nulo

    while (src < end && d < d_end) {
        if (*src == '+') {
            *d++ = ' '; // Reemplaza '+' con espacio
        } else if (*src == '%' && src + 2 < end) {
            // Decodifica caracteres codificados en %XX
            char hex[3] = { *(src + 1), *(src + 2), '\0' };
            *d++ = (char)strtol(hex, NULL, 16);
            src += 2;
        } else {
            *d++ = *src;
        }
        src++;
    }
    *d = '\0'; // Termina la cadena
}


/*
la siguiente funcion es un manejador para procesar los datos enviados
desde un formulario web y poder configurar la ESP32 como un access point
*/ 
esp_err_t submit_handler(httpd_req_t *req) {
    char param[100];
    if (httpd_req_get_url_query_str(req, param, sizeof(param)) == ESP_OK) {
        char ssid_encoded[50];
        char password_encoded[50];
        char ssid[50];
        char password[50];

        if (httpd_query_key_value(param, "SSID", ssid_encoded, sizeof(ssid_encoded)) == ESP_OK &&
            httpd_query_key_value(param, "PASSWORD", password_encoded, sizeof(password_encoded)) == ESP_OK) {

            // Decodificar SSID y contraseña
            url_decode(ssid_encoded, ssid, sizeof(ssid));
            url_decode(password_encoded, password, sizeof(password));

            ESP_LOGI(TAG, "SSID Decodificado: %s", ssid);
            ESP_LOGI(TAG, "PASSWORD Decodificado: %s", password);

            // Guardar credenciales en NVS
            save_wifi_config_to_nvs(WIFI_MODE_STA, ssid, password);

            // Cambiar a modo STA
            ESP_LOGI(TAG, "Modo AP detenido. Cambiando a modo STA...");
            wifi_init_sta(ssid, password);

            httpd_resp_sendstr(req, "Conectándose a la red especificada...");
        } else {
            httpd_resp_sendstr(req, "Error al procesar los datos del formulario.");
        }
    } else {
        httpd_resp_sendstr(req, "Error: No se recibieron parámetros.");
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

    set_led_color(COLOR_CYAN);

    ESP_LOGI(TAG, "Modo AP iniciado con SSID: %s, Contraseña: %s", AP_SSID, AP_PASSWORD);
}




// Modifica la función app_main para usar estas funciones
void app_main(void) {
    // Inicializar NVS
    if (borrar_memoria)
    {
        ESP_LOGI(TAG, "Borrando toda la memoria NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
        ESP_LOGI(TAG, "Memoria NVS borrada y reinicializada.");
    }else
    {
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ESP_ERROR_CHECK(nvs_flash_init());
        }

        // Inicializar LED RGB del ESP32
        init_led_strip();

        // Variables para las credenciales
        char ssid[32] = {0};
        char password[64] = {0};

        // Leer el estado de Wi-Fi desde NVS
        wifi_mode_t mode = read_wifi_config_from_nvs(ssid, sizeof(ssid), password, sizeof(password));

        if (mode == WIFI_MODE_STA && strlen(ssid) > 0 && strlen(password) > 0) {
            ESP_ERROR_CHECK(esp_netif_init());
            ESP_ERROR_CHECK(esp_event_loop_create_default());
            // Si hay credenciales, iniciar en modo STA
            wifi_init_sta(ssid, password);
        } else {
            // Si no hay credenciales, iniciar en modo AP
            init_spiffs();
            wifi_init_softap();
            start_http_server();
        }

        // Manejar eventos de Wi-Fi
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &event_handler,
            NULL,
            NULL
        ));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &event_handler,
            NULL,
            NULL
        ));
    }
}
