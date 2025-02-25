#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "led_strip.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "esp_timer.h"  // Librería para medir tiempo en microsegundos
#include "esp_adc/adc_oneshot.h"
#include "esp_wifi_types.h"
#include "lwip/sockets.h"


#define AP_SSID "RED ESP32 VICTOR"
#define AP_PASSWORD "12345678"
#define MAX_STA_CONN 4

#define GPIO_WS2812B 8
#define MAX_LEDS 1

#define COLOR_CYAN 0,255,255
#define COLOR_LIGHT_GREEN 0,255,0
#define COLOR_RED 255,0,0
#define COLOR_YELLOW 255,255,0
#define COLOR_MAGENTA 255, 0, 193
#define COLOR_ORANGE 255, 80, 40
#define TURN_OFF 0, 0, 0

#define NVS_WIFICONFIG "wifi_config"
#define NVS_KEY_MODE "mode"
#define NVS_KEY_SSID "ssid"
#define NVS_KEY_PASSWORD "password"

#define BUTTON_GPIO 18       // Pin GPIO donde está conectado el botón
#define HOLD_TIME_MS 10000   // Tiempo en milisegundos para considerar que el botón está presionado (10 segundos)

//parametros del ADC
#define SAMPLE_PERIOD_MS 100
#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_5
#define ADC_ATTENUATION ADC_ATTEN_DB_6
#define ADC_BITWIDTH ADC_BITWIDTH_12

static bool events_ip_registered = false;
static bool events_wifi_registered = false;

SemaphoreHandle_t nvs_mutex;

//QueueHandle_t stop_server_queue;

static led_strip_t *led_strip=NULL;

// handler del timer
static TimerHandle_t xTimerADC = NULL;

//handler del ADC
static adc_oneshot_unit_handle_t adc_handle = NULL;

//estado de conexion a red de internet
bool wifi_connected = false; 

//identificadores de handlers de eventos IP y WIFI:
esp_event_handler_instance_t instance_any_id;
esp_event_handler_instance_t instance_got_ip;

//estado de creación de interfaces STA y AP
static esp_netif_t *sta_netif = NULL;
static esp_netif_t *ap_netif = NULL;

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

esp_err_t root_handler(httpd_req_t *req);
esp_err_t submit_handler(httpd_req_t *req);
esp_err_t not_found_handler(httpd_req_t *req);
esp_err_t script_handler(httpd_req_t *req);
esp_err_t scan_handler(httpd_req_t *req);


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
 * Configura y registra un manejador para la ruta /submit la cual
 * se usa cuando el cliente presiona el boton "Enviar" en una pagina
 * web alojada en la ESP32. En este caso se procesa la solicitud con
 * submit_handle()
 */
httpd_uri_t submit = {
    .uri = "/submit",
    .method = HTTP_GET,
    .handler = submit_handler,
    .user_ctx = NULL
};

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

/**
 * Este registro permite servir un archivo script.js almacenado en 
 * SPIFFS si la página HTML de la ESP32 incluye:
 * 
 * <script src="/script.js"></script>
 */
httpd_uri_t script = {
    .uri = "/script.js",
    .method = HTTP_GET,
    .handler = script_handler,
    .user_ctx = NULL
};

/**
 * Esta ruta permite realizar un escaneo de redes wi-fi disponibles,
 * cuando el cliente accede a /scan, el manejador scan_handler() devuelve un JSON
 * con la lista de redes.
 */
httpd_uri_t scan = {
    .uri = "/scan",
    .method = HTTP_GET,
    .handler = scan_handler,
    .user_ctx = NULL
};

esp_err_t set_timer_adc(void);
esp_err_t start_timer_adc(void);
esp_err_t stop_timer_adc(void);
static void set_adc(void);

//variable de lectura del sensor
int adc_val = 0;

/**
 * @brief Este es un metodo que permite inicializar los GPIOS del ESP32. hasta el
 * momento solo se establece el GPIO 18 como entrada de pull down para poder conectar
 * un botón que pueda restablecer el modo AP de la ESP32 en caso de algun inconveniente
 * en el modo STA
 * 
 * @return nada
 */
void init_gpio() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO), // Seleccionar el pin
        .mode = GPIO_MODE_INPUT,               // Configurar como entrada
        .pull_up_en = GPIO_PULLUP_DISABLE,     // Deshabilitar pull-up
        .pull_down_en = GPIO_PULLDOWN_ENABLE,  // Habilitar pull-down
        .intr_type = GPIO_INTR_DISABLE         // Sin interrupciones
    };
    //Aplicar cambios
    esp_err_t ret = gpio_config(&io_conf);
    if (ret == ESP_OK) {
        ESP_LOGI("init_gpio", "GPIO %d configurado correctamente", BUTTON_GPIO);
    }
    else {
        ESP_LOGE("init_gpio", "Error al configurar GPIO: %d", ret);
    }
}

/**
 * @brief Esta funcion guarda configuraciones relacionadas con el modo Wi-Fi (STA 
 * o AP) y las credenciales Wi-FI (SSID y contraseña) en la memoria no volatil NVS 
 * del ESP32. Esto permite que la ESP32 recuerde el modo y las credenciales despues
 * de un reinicio. los parametros son los siguientes:
 * 
 * @param[in] mode Especifica el modo Wifi a guardar. Pueden ser dos modos: WIFI_MODE_STA
 * (modo estación) o WIFI_MODE_AP (modo punto de acceso).
 * 
 * @param[in] ssid cadena que contiene el nombre de la red Wi-Fi (SSID). Se guarda 
 * en NVS solo si el modo es WIFI_MODE_STA.
 * 
 * @param[in] password cadena que contiene la contraseña de la red Wi-Fi. También se 
 * guarda en NVS solo si el modo es WIFI_MODE_STA.
 */
void save_wifi_config_to_nvs(wifi_mode_t mode, const char *ssid, const char *password) {
    if (xSemaphoreTake(nvs_mutex, portMAX_DELAY) == pdTRUE){
        ESP_LOGI("save_wifi_config_to_nvs", "guardando credenciales...");
        /**
         * Lo siguiente es un identificador que se utiliza para interactuar con la memoria
         * no volatil.
         */
        esp_err_t err;
        nvs_handle_t nvs_handle;
    
        /**
         * Abre el namespace definido por NVS_WIFICONFIG (en este caso "wifi_config") en
         * modo lectura/escritura (NVS_READWRITE).
         */
        ESP_LOGI("save_wifi_config_to_nvs", "Abriendo espacio de memoria NVS");
        err = nvs_open(NVS_WIFICONFIG, NVS_READWRITE, &nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("save_wifi_config_to_nvs", "Error abriendo NVS: %s", esp_err_to_name(err));
            xSemaphoreGive(nvs_mutex);
            return;
        }
        ESP_LOGI("save_wifi_config_to_nvs", "Espacio de memoria NVS abierto correctamente");
        /**
         * Se guarda un valor del tipo uint8_t en la clave NVS_KEY_MODE ("mode") dentro del
         * espacio de nombres. el tercer parametro es el modo Wi-Fi actual el cual es manejado
         * por la variable wifi_mode_t mode la cual puede tomar los siguientes valores:
         * 
         * - WIFI_MODE_NULL = 0 -> modo nulo 
         * - WIFI_MODE_STA = 1 -> modo estación
         * - WIFI_MODE_AP = 2 -> modo punto de acceso
         * - WIFI_MODE_APSTA = 3 -> modo estación + punto de acceso
         * - WIFI_MODE_NAN = 4 -> modo NAN
         * - WIFI_MODE_MAX = 5 -> modo maximo
         * 
         * Sin embargo para este caso unicamente se manejan los valores 1 y 2.
         */
        ESP_LOGI("save_wifi_config_to_nvs", "Guardando en memoria NVS modo: %d", mode);
        err = nvs_set_u8(nvs_handle, NVS_KEY_MODE, mode);
        if (err != ESP_OK) {
            ESP_LOGE("save_wifi_config_to_nvs", "Error guardando modo Wi-Fi: %s", esp_err_to_name(err));
            nvs_close(nvs_handle);
            xSemaphoreGive(nvs_mutex);
            return;
        }
        ESP_LOGI("save_wifi_config_to_nvs", "modo %d guardado correctamente", mode);
        
        /**
         * Verifica si el modo es WIFI_MODE_STA y si las cadenas ssid y password no son
         * NULL guarda las cadenas en las claves correspondientes de NVS
         */
        if (mode == WIFI_MODE_STA && ssid && password) {
            if (strlen(ssid) > 31) {
                ESP_LOGE("save_wifi_config_to_nvs", "Error: SSID demasiado largo (%d caracteres)", strlen(ssid));
                nvs_close(nvs_handle);
                xSemaphoreGive(nvs_mutex);
                return;
            }
            if (strlen(password) > 63) {
                ESP_LOGE("save_wifi_config_to_nvs", "Error: Password demasiado largo (%d caracteres)", strlen(password));
                nvs_close(nvs_handle);
                xSemaphoreGive(nvs_mutex);
                return;
            }
    
            ESP_LOGI("save_wifi_config_to_nvs", "guardando SSID: %s", ssid);
            err = nvs_set_str(nvs_handle, NVS_KEY_SSID, ssid);
            if (err != ESP_OK) {
                ESP_LOGE("save_wifi_config_to_nvs", "Error guardando SSID: %s", esp_err_to_name(err));
                nvs_close(nvs_handle);
                xSemaphoreGive(nvs_mutex);
                return;
            }
            ESP_LOGI("save_wifi_config_to_nvs", "SSID guardado correctamente");
    
            ESP_LOGI("save_wifi_config_to_nvs", "guardando PASSWORD");
            err = nvs_set_str(nvs_handle, NVS_KEY_PASSWORD, password);
            if (err != ESP_OK) {
                ESP_LOGE("save_wifi_config_to_nvs", "Error guardando PASSWORD: %s", esp_err_to_name(err));
                nvs_close(nvs_handle);
                xSemaphoreGive(nvs_mutex);
                return;
            }
            ESP_LOGI("save_wifi_config_to_nvs", "PASSWORD guardado correctamente");
        }
        
        /**
         * Se confirman los cambios en la memoria flash
         */
        ESP_LOGI("save_wifi_config_to_nvs", "aplicando cambios en memoria NVS...");
        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("save_wifi_config_to_nvs", "Error confirmando cambios en NVS: %s", esp_err_to_name(err));
            nvs_close(nvs_handle);
            xSemaphoreGive(nvs_mutex);
            return;
        }
        /**
         * Se cierra el espacio de nombres
         */
        ESP_LOGI("save_wifi_config_to_nvs", "cambios aplicados en memoria NVS");
        ESP_LOGI("save_wifi_config_to_nvs", "cerrando memoria nvs...");
        nvs_close(nvs_handle);
        ESP_LOGI("save_wifi_config_to_nvs", "memoria nvs cerrada correctamente");
        
        vTaskDelay(pdMS_TO_TICKS(500));
        
        ESP_LOGI("save_wifi_config_to_nvs", "Liberando mutex...");
        xSemaphoreGive(nvs_mutex);
        ESP_LOGI("save_wifi_config_to_nvs", "mutex liberado correctamente");
    }else{
        ESP_LOGE("save_wifi_config_to_nvs", "No se pudo obtener el MutEx. abortando escritura en NVS...");
    }
}

void clear_wifi_credentials_from_nvs(void) {
    nvs_handle_t nvs_handle;

    // Abrir el namespace en modo lectura/escritura
    esp_err_t err = nvs_open(NVS_WIFICONFIG, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("clear_wifi_credentials_from_nvs", "Error abriendo NVS: %s", esp_err_to_name(err));
        return;
    }else{
        ESP_LOGI("clear_wifi_credentials_from_nvs", "NVS abierto correctamente");
    }

    // Eliminar las claves de configuración Wi-Fi
    err = nvs_erase_key(nvs_handle, NVS_KEY_MODE);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE("clear_wifi_credentials_from_nvs", "Error eliminando clave MODE: %s", esp_err_to_name(err));
    }else{
        ESP_LOGI("clear_wifi_credentials_from_nvs", "Clave MODE eliminada correctamente");
    }

    err = nvs_erase_key(nvs_handle, NVS_KEY_SSID);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE("clear_wifi_credentials_from_nvs", "Error eliminando clave SSID: %s", esp_err_to_name(err));
    }else{
        ESP_LOGI("clear_wifi_credentials_from_nvs", "Clave SSID eliminada correctamente");
    }

    err = nvs_erase_key(nvs_handle, NVS_KEY_PASSWORD);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE("clear_wifi_credentials_from_nvs", "Error eliminando clave PASSWORD: %s", esp_err_to_name(err));
    }else{
        ESP_LOGI("clear_wifi_credentials_from_nvs", "Clave PASSWORD eliminada correctamente");
    }
    

    // Aplicar cambios
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("clear_wifi_credentials_from_nvs", "Error confirmando cambios en NVS: %s", esp_err_to_name(err));
    }else{
        ESP_LOGI("clear_wifi_credentials_from_nvs", "Cambios confirmados en NVS");
    }

    // Cerrar NVS
    nvs_close(nvs_handle);

    ESP_LOGI("clear_wifi_credentials_from_nvs", "Credenciales Wi-Fi eliminadas correctamente");
}


/**
 * @brief La siguiente función permite leer de la memoria no volatil (NVS) el modo
 * Wi-Fi, así como el SSID y la contreseña si el dispositivo está configurado en modo
 * estación (STA).
 * 
 * @param[in] ssid Puntero a una cadena donde se almacenará el SSID leído desde la 
 * NVS. Se pasa vacío desde el programa principal y se llena con el valor almacenado.
 * @param[in] ssid_len Tamaño maximo del buffer donde se guardará el SSID.
 * @param[in] password Puntero a una cadena donde se almacenará la contraseña leída 
 * desde la NVS.
 * @param[in] password_len Tamaño máximo del buffer donde se guardará la contraseña
 * 
 * @return Devuelve el modo Wi-Fi almacenado en NVS (WIFI_MODE_AP o WIFI_MODE_STA). Si el
 * modo es WIFI_MODE_STA, también rellena las variables ssid y password con las credenciales
 * de la red Wi-Fi previamente guardadas. 
 */
wifi_mode_t read_wifi_config_from_nvs(char *ssid, size_t ssid_len, char *password, size_t password_len) {
    // se declara el identificador para interactuar con NVS
    nvs_handle_t nvs_handle;

    /**
     * Inicializa mode como WIFI_MODE_AP. Esto asegura que, en caso de fallo al leer
     * desde NVS, el dispositivo inicie en modo Acces Point (AP).
     */
    wifi_mode_t mode = WIFI_MODE_AP;

    /**
     * Se intenta abrir el namespace NVS_WIFICONFIG en modo solo lectura (NVS_READONLY).
     * Si ocurre un error (por ejemplo, el namespace no existe), la función continúa sin
     * intentar leer.
     */
    esp_err_t err = nvs_open(NVS_WIFICONFIG, NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        uint8_t stored_mode;
        /**
         * nvs_get_u8 lee un valor tipo uint8_t asociado a la clave NVS_KEY_MODE. Si
         * la lectura es exitosa, el valor leído (stored_mode) se convierte a wifi_mode_t
         * y se almacena en mode.
         */
        err = nvs_get_u8(nvs_handle, NVS_KEY_MODE, &stored_mode);
        if (err == ESP_OK) {
            ESP_LOGI("read_wifi_config_from_nvs", "clave MODE leida correctamente: %d", (wifi_mode_t)stored_mode);
            mode = (wifi_mode_t)stored_mode;
        }else
        {
            ESP_LOGE("read_wifi_config_from_nvs", "Error leyendo clave MODE: %s", esp_err_to_name(err));
        }
        

        /**
         * Si el modo leido es WIFI_MODE_STA se lee la cadena asociada a NVS_KEY_SSID con
         * la funcion nvs_get_str() y la almacena en el buffer ssid. el tamaño del buffer debe
         * especificarse mediante ssid_len. Lo mismo ocurre para la contraseña (NVS_KEY_PASSWORD)
         * y el buffer password.
         */
        if (mode == WIFI_MODE_STA) {
            err = nvs_get_str(nvs_handle, NVS_KEY_SSID, ssid, &ssid_len);
            
            if (err == ESP_OK) {
                ESP_LOGI("read_wifi_config_from_nvs", "Clave SSID leida correctamente: %s", ssid);
            } else {
                ESP_LOGE("read_wifi_config_from_nvs", "Error leyendo clave SSID: %s", esp_err_to_name(err));
            }
            
            err = nvs_get_str(nvs_handle, NVS_KEY_PASSWORD, password, &password_len);
            if (err == ESP_OK) {
                ESP_LOGI("read_wifi_config_from_nvs", "Clave PASSWORD leida correctamente: %s", password);
            }else{
                ESP_LOGE("read_wifi_config_from_nvs", "Error leyendo clave PASSWORD: %s", esp_err_to_name(err));
            }
        }

        /**
         * Finalmente se cierra la memoria no volatil.
         */
        nvs_close(nvs_handle);
    }else
    {
        ESP_LOGE("read_wifi_config_from_nvs", "Error abriendo NVS: %s", esp_err_to_name(err));
    }
    

    /**
     * Se devuelve el modo Wi-Fi que se leyó desde la NVS. Si no se pudo leer nada, devolverá
     * WIFI_MODE_AP como valor predeterminado.
     */
    return mode;
}

/**
 * @brief La función init_led_strip inicializa el controlador para manejar un LED RGB basado en 
 * tiras LED WS2812B. En este caso unicamente se dispone de un LED integrado en la ESP32.
 * 
 * Esta función tambien asegura que el LED comience apagado.
 */
void init_led_strip(void){
    /**
     * La función led_strip_init permite inicializar el controlador LED. Este tiene
     * los siguientes parametros:
     * 
     * @param[in] channel Especifica el canal RMT (Remote Control) que usará el ESP32
     * para controlar las señales del LED. En este caso, el canal 0 se asigna al 
     * controlador. Es importante aclarar que el canal RMT es un periferico especializado
     * del ESP32 diseñado para manejar señales temporales precisas, como las que utilizan
     * en protocolos de comunicación con temporización estricta. es especialmente util
     * para generar y recibir señales moduladas con precisión.
     * 
     * La ESP32-C6-DevKitC-1 tiene en total 4 canales:
     * 
     * (canales 0-1) canales TX o de transmision.
     * (canales 2-3) canales RX o de recepción.
     * 
     * @param[in] gpio Especifica el número del pin GPIO al que está conectado el LED 
     * o la tira LED. en este caso se usa el GPIO 8 al cual esta conectado el WS2812B.
     * 
     * @param[in] led_num indica el numero de LEDs que se controlarán en la tira. en 
     * este caso se establece en 1 ya que solo se manejará un solo led.
     * 
     * @return La funcion devuelve un puntero a una estructura led_strip_t que
     * representa el controlador del LED. Si la inicialización falla devuelve NULL.
     */
    led_strip = led_strip_init(0, GPIO_WS2812B, MAX_LEDS);

    /**
     * Si la inicialización del led falla se imprime un mensaje de error y se sale de 
     * la función usando return.
     */
    if (!led_strip)
    {
        ESP_LOGE("init_led_strip", "Fallo al inicializar el LED RGB");
        return;
    }

    /**
     * Se apaga el LED configurandolo en color negro (0,0,0). Esta función toma como 
     * parametros:
     * 
     * @param[in] led_strip El controlador inicializado en pasos anteriores.
     * @param[in] timeout_ms Tiempo maximo de espera en milisegundos para realizar la accion.
     * en este caso se establece en 100ms para evitar que se dejen de ejecutar las 
     * otras tareas que esta realizando el ESP32 si ocurre un daño en el LED.
     * 
     * @note hay que tener en cuenta que la sintaxis -> indica que se esta dereferenciando
     * un puntero a una estructura y accediendo a la función clear() de esa estructura.
     */
    ESP_ERROR_CHECK(led_strip->clear(led_strip, 100));
}

/**
 * @brief Esta función establece el estado de color del LED RGB integrado en el ESP32
 * 
 * @param[in] red Intensidad del color rojo (0->255).
 * @param[in] green Intensidad del color verde (0->255).
 * @param[in] blue Intensidad del color azul (0->255).
 */
void set_led_color(uint8_t red, uint8_t green, uint8_t blue){
    /**
     * Si se ha inicializado correctamente el LED se establece el color del LED
     */
    if (led_strip)
    {
        /**
         * La funcion set_pixel permite establecer el color del pixel. esta tiene 
         * los siguientes parametros:
         * 
         * @param[in] strip la estructura que contiene la inicialización del LED
         * @param[in] index el indice del LED al cual se le va a cambiar el color.
         * en este caso como solo se esta usando el LED de la ESP32 este indice es
         * el del primer LED, es decir, 0.
         * @param[in] red Intensidad del color rojo (0->255).
         * @param[in] green Intensidad del color verde (0->255).
         * @param[in] blue Intensidad del color azul (0->255).
         */
        ESP_ERROR_CHECK(led_strip->set_pixel(led_strip,0,red,green,blue));
        /**
         * Se actualiza el estado del LED RGB con las configuraciones creadas. se
         * establece un timeout de 100ms para evitar que el ESP32 se quede ejecutando
         * la tarea de actualizar led si ocurre un error. esto es importante para 
         * evitar que el servidor se caiga en caso de que ocurra un daño en el LED.
         */
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
        ESP_LOGE("init_spiffs", "Error al inicializar SPIFFS (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI("init_spiffs", "SPIFFS inicializado correctamente");
    }
}

/**
 * @brief Este es un manejador de eventos llamado event_handler que se encarga de 
 * procesar eventos relacionados con Wi-Fi e IP. Este tipo de función es fundamental
 * para controlar el flujo de trabajo en la ESP32 respondiendo a cambios de estado
 * de red. Este manejador tiene los siguientes parametros:
 * 
 * @param[in] arg un puntero generico que se puede usar para pasar datos personalizados
 * al manejador. En este caso no se usa explicitamente
 * @param[in] event_base se especifica la categoria del evento. Los valores mas comunes
 * son WIFI_EVENT (relacionados con Wi-Fi) e IP_EVENT (relacionados con la red IP)
 * @param[in] event_id Identifica el tipo especifico de evento dentro de la categoria
 * indicada por event_base. Por ejemplo, para WIFI_EVENT, los IDs pueden ser
 * WIFI_EVENT_START, WIFI_EVENT_DISCONNECTED, etc...
 * @param[in] event_data Puntero a una estructura que contiene datos adicionales
 * relacionados con el evento. Por ejemplo, en el caso de IP_EVENT_STA_GOT_IP, contiene
 * información de la direccion IP obtenida. 
 */
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    
    /**
     * Este evento ocurre cuando el dispositivo pierde la conexión Wi-Fi o no puede
     * conectarse a la red especificada.
     */ 
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        /**
         * Registra una advertencia en el LOG con un mensaje que indica "Conexión 
         * conexion fallida, intentando reconectar..."
         */
        ESP_LOGI("event_handler", "Conexión fallida, intentando reconectar...");
        wifi_connected = false;

        /**
         * Se llama nuevamente a esp_wifi_connect() para reintentar la conexión.
         */
        if(esp_wifi_connect()==ESP_OK){
            ESP_LOGI("event_handler", "Conectado correctamente");
        }else{
            ESP_LOGE("event_handler", "Error al conectar");
        } // Reintentar conexión

        /**
         * Se cambia el color del LED RGB a rojo (COLOR RED), indicando que la conexion
         * ha fallado.
         */
        set_led_color(COLOR_RED);
        //apaga la lectura del ADC
        ESP_ERROR_CHECK(stop_timer_adc());
    } 
    /**
     * Este evento ocurre cuando el dispositivo ha obtenido una dirección IP valida del
     * router después de conectarse exitosamente.
     */
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        /**
         * Convierte event_data a un puntero del tipo ip_event_got_ip_t para acceder a
         * los datos relacionados con la IP obtenida. 
         */
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        /**
         * Cuando el ESP32 obtiene una dirección IP valida (por ejemplo a traves de DHCP
         * en modo estación), esta dirección esta almacenada en una estructura especial.
         * Para mostrar esta dirección IP como una cadena legible (x.x.x.x), se necesita
         * convertirla desde su formato binario a uno textual. aqui es donde entran las
         * macros IPSTR y IP2STR:
         * 
         * IPSTR se traduce a: "%d.%d.%d.%d". esto significa que espera cuatro valores
         * enteros, uno por cada octeto.arg
         * IP2STR es  otra macro que convierte una direccion IP almacenada en formato
         * binario (tipo ip4_addr_t) en 4 enteros separados, uno por cada octeto.
         * Finalmente, event->ip_info.ip es el dato que se quiere rescatar. este se compone
         * de la siguiente forma:arg
         * event -- es un puntero al dato del evento recibido (ip_event_got_ip)
         * event->ip_info -- es una estructura que contiene información sobre la conexón IP.
         * event->ip_info.ip es la direccion IP obtenida en formato binario
         * 
         */
        ESP_LOGI("event_handler", "Conexión exitosa, dirección IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_mode_t mode = WIFI_MODE_NULL; 
        if (esp_wifi_get_mode(&mode) != ESP_OK) {
            ESP_LOGW("wifi_set_STA", "No se pudo obtener el modo Wi-Fi. Asegurar que Wi-Fi está iniciado.");
        }else{
            if(mode == WIFI_MODE_STA){
                //enciende la lectura del ADC
                ESP_ERROR_CHECK(start_timer_adc());
                /**
                 * Se establece el led RGB de color verde para indicar que la conexion fue exitosa
                 */
                set_led_color(COLOR_LIGHT_GREEN);
            }
        }
        wifi_connected = true;
    }
}

/**
 * @brief Establecer modo estación
 * 
 * La función wifi_set_STA() configura la ESP32 en modo estación (STA - Station Mode), 
 * permitiéndole conectarse a una red Wi-Fi existente. Esta función gestiona correctamente 
 * los posibles estados del Wi-Fi y garantiza que la configuración se realice de manera 
 * estable y sin conflictos.
 */
void wifi_set_STA(void){

    if (xSemaphoreTake(nvs_mutex, portMAX_DELAY) == pdTRUE){
        /**
         * Imprime en el monitor serie un mensaje informativo indicando que el proceso de 
         * configuración del modo estación está iniciando.
         */
        ESP_LOGI("wifi_set_STA", "Configurando Wi-Fi en modo STA...");
        /**
         * Se declara la variable mode e inicialmente se establece en WIFI_MODE_NULL 
         * (sin modo configurado).
         */
        wifi_mode_t mode = WIFI_MODE_NULL; 
    
        /**
         * Se llama a esp_wifi_get_mode(&mode), que obtiene el modo Wi-Fi actual de 
         * la ESP32.
         * 
         * en caso que la función falle, se imprime una advertencia de que el wifi no ha 
         * sido iniciado.
         */
        if (esp_wifi_get_mode(&mode) != ESP_OK) {
            ESP_LOGW("wifi_set_STA", "No se pudo obtener el modo Wi-Fi. Asegurar que Wi-Fi está iniciado.");
        }
    
        /**
         * Solamente si sta_netif no ha sido creada aún (NULL), se crea la interfaz de red para el 
         * modo STA usando esp_netif_create_default_wifi_sta().
         * 
         * Esto evita crear múltiples interfaces, lo que podría generar fugas de memoria.
         */
        if (!sta_netif) {
            ESP_LOGI("wifi_set_STA", "No se ha creado la interfaz. Creando interfaz STA...");
            sta_netif = esp_netif_create_default_wifi_sta();
            if(sta_netif){
                ESP_LOGI("wifi_set_STA", "Interfaz STA creada correctamente.");
            }else
            {
                ESP_LOGW("wifi_set_STA", "Error al crear interfaz STA.");
            }
            
        }else{
            ESP_LOGW("wifi_set_STA", "La interfaz STA ya estaba creada");
        }
    
        // **Si Wi-Fi no está configurado en ningún modo, forzar modo STA**
        if (mode == WIFI_MODE_NULL) {
            /**
             * Si mode == WIFI_MODE_NULL significa que el Wi-Fi no tiene ningún modo activo.
             * 
             * Se imprime una advertencia y se configura el modo Wi-Fi en STA con 
             * esp_wifi_set_mode(WIFI_MODE_STA). Luego, se inicia el Wi-Fi con esp_wifi_start().
             * 
             * Esta acción es necesaria puesto que si la ESP32 no tiene configurado el Wi-Fi, 
             * cualquier intento de conexión fallará. con esta solución se asegura que la ESP32 
             * esté lista para conectarse a una red.
             */
            ESP_LOGW("wifi_set_STA", "Wi-Fi no tiene un modo configurado. Estableciendo modo STA...");
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_start());
            ESP_LOGI("wifi_set_STA", "Wi-Fi iniciado en modo STA");
        } else if (mode != WIFI_MODE_STA) {
            /**
             * Si la ESP32 está en modo AP (WIFI_MODE_AP) o AP+STA (WIFI_MODE_APSTA), se cambia
             * a STA usando esp_wifi_set_mode(WIFI_MODE_STA) sin volver a iniciar wifi con 
             * esp_wifi_start(). Esto evita problemas de conectividad en caso de que la ESP32 
             * haya sido configurada en otro modo previamente.
             */
            ESP_LOGI("wifi_set_STA", "Deteniendo Wi-Fi antes de cambiar a STA...");
            ESP_ERROR_CHECK(esp_wifi_stop());
            vTaskDelay(pdMS_TO_TICKS(1000));
            
            ESP_LOGW("wifi_set_STA", "Cambiando Wi-Fi a modo STA...");
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_start());
            ESP_LOGW("wifi_set_STA", "Wi-Fi establecido en modo STA");
        } else {
            /**
             * Si mode == WIFI_MODE_STA, significa que la ESP32 ya está en modo STA.
             * En este caso, solo se inicia el Wi-Fi con esp_wifi_start(), sin necesidad 
             * de cambiar el modo.
             * 
             * De esta manera se evita reiniciar innecesariamente la configuración si la 
             * ESP32 ya está en el modo correcto.
             */
            ESP_LOGI("wifi_set_STA", "Wi-Fi ya está en modo STA");
            ESP_ERROR_CHECK(esp_wifi_start());
            ESP_LOGI("wifi_set_STA", "Wi-Fi iniciado");
        }
        ESP_LOGI("wifi_set_STA", "Liberando mutex...");
        xSemaphoreGive(nvs_mutex);
        ESP_LOGI("wifi_set_STA", "mutex liberado");
    }else{
        ESP_LOGW("wifi_set_STA", "No se pudo obtener el mutex en el tiempo límite, abortando configuración de Wi-Fi.");
    }
}

void start_IP_events(void){
    
    if(!events_ip_registered){
        ESP_LOGI("start_IP_events", "Iniciando eventos IP...");
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &event_handler,
            NULL,
            &instance_got_ip
        ));
        events_ip_registered = true;
        ESP_LOGI("start_IP_events", "Eventos IP iniciados");
    }
    
}

/**
 * @brief Iniciar eventos Wi-Fi
 * 
 * La función start_WIFI_events() registra un manejador de eventos de Wi-Fi 
 * en la ESP32, permitiendo que la función event_handler() reciba y procese eventos 
 * relacionados con Wi-Fi (conexión, desconexión, escaneo, etc.).
 */
void start_WIFI_events(void){
    /**
     * Registra un manejador de eventos, permitiendo que la ESP32 detecte y procese 
     * eventos específicos.
     * Esto significa que cada vez que ocurra un evento de Wi-Fi, la función event_handler() 
     * será ejecutada.
     */
    if(!events_wifi_registered){
        ESP_LOGI("start_WIFI_events", "Iniciando eventos Wi-Fi...");
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &event_handler,
            NULL,
            &instance_any_id
        ));
        events_wifi_registered=true;
        ESP_LOGI("start_WIFI_events", "Eventos Wi-Fi iniciados");
    }

}
esp_err_t stop_IP_events() {
    if (events_ip_registered) {
        esp_err_t err = esp_event_handler_instance_unregister(IP_EVENT, 
            IP_EVENT_STA_GOT_IP, 
            instance_got_ip
        );
        if (err == ESP_OK) {
            ESP_LOGI("stop_IP_events", "Evento IP_EVENT_STA_GOT_IP desregistrado exitosamente.");
            events_ip_registered = false;
        } else {
            ESP_LOGE("stop_IP_events", "Error al desregistrar evento: %s", esp_err_to_name(err));
        }
        return err;
    }
    return ESP_ERR_INVALID_STATE;
}


esp_err_t stop_WIFI_events() {
    if (events_wifi_registered) {
        esp_err_t err = esp_event_handler_instance_unregister(WIFI_EVENT, 
            ESP_EVENT_ANY_ID, 
            instance_any_id);
        if (err == ESP_OK) {
            ESP_LOGI("stop_WIFI_events", "Evento WIFI_EVENT_ANY_ID desregistrado exitosamente.");
            events_wifi_registered = false;
        } else {
            ESP_LOGE("stop_WIFI_events", "Error al desregistrar evento Wi-Fi: %s", esp_err_to_name(err));
        }
        return err;
    }
    return ESP_ERR_INVALID_STATE;
}


/**
 * @brief conectase a una red (solo para modo STA)
 * 
 * La función wifi_connect_STA() intenta conectar la ESP32 a una red Wi-Fi en 
 * modo estación (STA) utilizando el SSID y la contraseña proporcionados. También 
 * gestiona el proceso de conexión, incluyendo validaciones, configuración de 
 * parámetros y espera hasta que la conexión sea establecida o falle después de 
 * un tiempo límite.
 * 
 * @param[in] ssid nombre de la red a la cual debe conectarse
 * @param[in] password contraseña a la cual debe conectarse
 * 
 * @return retorna ESP_OK si la conexión fue exitosa, o ESP_FAIL en caso de fallo
 */
esp_err_t wifi_connect_STA(const char *ssid, const char *password) {
    
    /**
     * Imprime en la terminal el nombre de la red a la que se intentará conectar.
     * Esto facilita la depuración y el seguimiento del estado de conexión.
     */
    ESP_LOGI("wifi_connect_STA", "Conectando a la red Wi-Fi: %s con la contraseña %s...", ssid, password);
    
    /**
     * Se obtiene el modo Wi-Fi actual usando esp_wifi_get_mode(&mode).
     * Si el Wi-Fi no está en modo STA o AP+STA, se muestra un error y 
     * se cancela la conexión. Esto previene errores al intentar 
     * conectarse si la ESP32 está en modo AP (WIFI_MODE_AP)
     */
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if (mode != WIFI_MODE_STA && mode != WIFI_MODE_APSTA) {
        ESP_LOGE("wifi_connect_STA", "Error: Wi-Fi no está en modo STA o AP+STA. No se puede conectar.");
        return ESP_FAIL;
    }

    /**
     * Se crea una estructura wifi_config_t que almacena los datos de conexión Wi-Fi en sta (modo estación).
     * Inicialmente, los campos ssid y password están vacíos.
     */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
        },
    };

    /**
     * Se copian los valores de ssid y password en la estructura wifi_config.sta de forma 
     * segura usando strncpy(), lo que evita desbordamiento de memoria.
     */
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    /**
     * Se aplica la configuración Wi-Fi al interfaz de estación (WIFI_IF_STA) con 
     * esp_wifi_set_config().Esto almacena el SSID y la contraseña en el controlador Wi-Fi 
     * de la ESP32.
     */
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    /**
     * Llama a start_IP_events(), que registra eventos relacionados 
     * con la asignación de direcciones IP.
     */
    ESP_LOGI("wifi_connect_STA", "Memoria libre antes de start_IP_events: %lu", (unsigned long)esp_get_free_heap_size());
    start_IP_events();
    ESP_LOGI("wifi_connect_STA", "Memoria libre después de start_IP_events: %lu", (unsigned long)esp_get_free_heap_size());
    /**
     * Se muestra un mensaje en la terminal indicando que la ESP32 está intentando conectar.
     */
    ESP_LOGI("wifi_connect_STA", "Esperando conexión...");
    
    /**
     * El LED se pone en amarillo (COLOR_YELLOW) para indicar el estado de conexión 
     * en curso.
     */
    set_led_color(COLOR_YELLOW);

    //Se inicia el proceso de conexión Wi-Fi con esp_wifi_connect()

    esp_err_t err = esp_wifi_connect();

    /**
     * Se implementa un bucle que espera hasta 10 segundos (10 
     * intentos de 1 segundo) para verificar si la conexión fue exitosa.
     * 
     * Cada iteración espera 1 segundo (vTaskDelay(pdMS_TO_TICKS(1000))) 
     * antes de volver a comprobar el estado de wifi_connected.
     * 
     * wifi_connected es una variable global que se actualiza 
     * cuando la ESP32 se conecta correctamente.
     */
    
    int intentos = 0;
    while (!wifi_connected && intentos < 10) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Esperar 1 segundo
        intentos++;
    }

    /**
     * Si wifi_connected == true después de esperar, significa 
     * que la conexión fue exitosa. En tal caso se inicia start_WIFI_events(), 
     * lo que permite manejar eventos Wi-Fi como desconexión o reconexión y se
     * culmina retornando ESP_OK.
     * 
     * Si la conexión no se estableció después de 10 segundos, se imprime una 
     * advertencia y la función retorna ESP_FAIL.
     */
    if (wifi_connected && (err==ESP_OK)) {
        start_WIFI_events();
        ESP_LOGI("wifi_connect_STA", "Conexión exitosa a: %s", ssid);
        return ESP_OK;
    } else {
        ESP_LOGW("wifi_connect_STA", "No se pudo conectar a: %s, con la contraseña: %s", ssid, password);
        return ESP_FAIL;
    }
}

/**
 * @brief Esta función se usa para decodificar cadenas codificadas en formato URL encoding.
 * El URL encoding es un esquema en el que ciertos caracteres especiales se 
 * representan como %XX, donde XX es el valor hexadecimal del caracter.
 * 
 * Una de las funciones es reemplazar + por un espacio. otra es reemplazar %XX por
 * el caracter correspondiente al codigo hexadecimal.
 * 
 * @param[in] src cadena de entrada codificada en URL encoding
 * @param[out] dest Buffer de salida donde se almacenará la cadena decodificada
 * @param[in] dest_size Tamaño maximo del buffer dest. se usa para evitar desbordamiento
 * de memoria.
 * 
 * 
 */
void url_decode(char *src, char *dest, size_t dest_size) {
    /**
     * Se apunta al final de la cadena src. esto permite iterar hasta el final de
     * la cadena de entrada. para entender porque sumar src + el tamaño de la cadena
     * retorna la posicion final de src es necesario entender como se maneja internamente
     * las cadenas de caracteres en la memoria del ESP32. se recomienda revisar la 
     * documentacion al respecto: [colocar enlace de explicación] 
     */
    char *end = src + strlen(src);
    /**
     * Se apunta al inicio del buffer de destino
     */
    char *d = dest;
    /**
     * Se apunta al ultimo espacio valido en el buffer dest, dejando espacio para el 
     * terminador nulo \0
     */
    char *d_end = dest + dest_size - 1;

    /**
     * Mientras  no se alcance el final de la cadena de entrada (src < end) y no se
     * llene el buffer de destino (d < d_end)
     */
    while (src < end && d < d_end) {
        /**
         * Sí el caracter es equivalente a + se reemplaza con un espacio
         */
        if (*src == '+') {
            *d++ = ' '; 
        } 
        /**
         * Sí el caracter es igual a % y 2 posiciones mas alante no se encuentra
         * el final de la cadena de entrada se convierte el caracter a su valor 
         * decimal correspondiente , escribiendolo en el buffer destino.
         */
        else if (*src == '%' && src + 2 < end) {
            /**
             * Se extraen los dos caracteres siguientes en la cadena hex de 3 bytes 
             * agregando el caracter \0 para asegurar que hex sea una cadena valida
             * para las funciones de conversión.
             */
            char hex[3] = { *(src + 1), *(src + 2), '\0' };
            /**
             * la función strtol() interpreta hex como un numero en base 16 y lo
             * convierte a su valir decimal correspondiente. este valor se escribe 
             * en el buffer destino.
             * 
             * Para entender esto mejor debe saber que las cadenas de caracteres
             * manejan codigo ASCII que asocia numeros en base 10 a las letras, 
             * simbolos, signos de puntuacion, caracteres especiales, etc...
             * 
             * por ejemplo si del navegador llega el codigo %20 se convierte el
             * numero 20 en hexadecimal a su correspondiente decimal (32) el cual
             * a su vez corresponde a un espacio en codigo ASCII.
             */
            *d++ = (char)strtol(hex, NULL, 16);
            /**
             * Se incrementa src en 2 posiciones.
             */
            src += 2;
        } 
        /**
         * Si el caracter actual no es ni + ni % simplemente se copia al buffer
         * destino sin modificaciones.
         */
        else {
            *d++ = *src;
        }
        /**
         * se incrementa una posicion de la cadena de caracteres
         */
        src++;
    }
    *d = '\0'; // Termina la cadena con el caracter nulo
}

void stop_http_server() {
    if (server != NULL) {
        if (xSemaphoreTake(nvs_mutex, pdMS_TO_TICKS(5000)) == pdTRUE){
            for (int fd = 0; fd < CONFIG_LWIP_MAX_SOCKETS; fd++) {
                esp_err_t err = httpd_sess_trigger_close(server, fd);
                if (err == ESP_OK) {
                    ESP_LOGI("stop_http_server", "Conexión en socket %d cerrada correctamente.", fd);
                } else if (err == ESP_ERR_NOT_FOUND) {
                    ESP_LOGW("stop_http_server", "Socket %d no estaba en uso.", fd);
                } else {
                    ESP_LOGE("stop_http_server", "Error cerrando socket %d: %s", fd, esp_err_to_name(err));
                }
            }
    
            wifi_sta_list_t sta_list;
            esp_wifi_ap_get_sta_list(&sta_list);
    
            if (sta_list.num > 0) {
                ESP_LOGW("stop_http_server", "Hay %d clientes conectados. Desconectándolos antes de apagar el AP...", sta_list.num);
    
                for (int i = 0; i < sta_list.num; i++) {
                    esp_err_t err = esp_wifi_deauth_sta(i + 1); // AID comienza desde 1
                    if (err == ESP_OK) {
                        ESP_LOGI("stop_http_server", "Cliente %d (%02X:%02X:%02X:%02X:%02X:%02X) desconectado.",
                                i,
                                sta_list.sta[i].mac[0], sta_list.sta[i].mac[1], sta_list.sta[i].mac[2],
                                sta_list.sta[i].mac[3], sta_list.sta[i].mac[4], sta_list.sta[i].mac[5]);
                    } else {
                        ESP_LOGE("stop_http_server", "Error desconectando al cliente %d (%02X:%02X:%02X:%02X:%02X:%02X): %s",
                                i,
                                sta_list.sta[i].mac[0], sta_list.sta[i].mac[1], sta_list.sta[i].mac[2],
                                sta_list.sta[i].mac[3], sta_list.sta[i].mac[4], sta_list.sta[i].mac[5],
                                esp_err_to_name(err));
                    }
                }
            } else {
                ESP_LOGI("stop_http_server", "No hay clientes conectados.");
            }
    
            // **Intentar detener el servidor HTTP**
            if(httpd_unregister_uri_handler(server, "/", HTTP_GET)==ESP_OK){
                ESP_LOGI("stop_http_server","ruta / borrada correctamente");
            }else{
                ESP_LOGE("stop_http_server","ruta / no se pudo borrar correctamente");
            }
    
            if(httpd_unregister_uri_handler(server, "/submit", HTTP_GET)==ESP_OK){
                ESP_LOGI("stop_http_server","ruta /submit borrada correctamente");
            }else{
                ESP_LOGE("stop_http_server","ruta /submit no se pudo borrar correctamente");
            }
    
            if(httpd_unregister_uri_handler(server, "/*", HTTP_GET)==ESP_OK){
                ESP_LOGI("stop_http_server","ruta /* borrada correctamente");
            }else{
                ESP_LOGE("stop_http_server","ruta /* no se pudo borrar correctamente");
            }
    
            if(httpd_unregister_uri_handler(server, "/script.js", HTTP_GET)==ESP_OK){
                ESP_LOGI("stop_http_server","ruta /script.js borrada correctamente");
            }else{
                ESP_LOGE("stop_http_server","ruta /script.js no se pudo borrar correctamente");
            }
    
            if(httpd_unregister_uri_handler(server, "/scan", HTTP_GET)==ESP_OK){
                ESP_LOGI("stop_http_server","ruta /scan borrada correctamente");
            }else{
                ESP_LOGE("stop_http_server","ruta /scan no se pudo borrar correctamente");
            }
            
            ESP_LOGI("stop_http_server", "Deteniendo servidor HTTP...");
            esp_err_t err = httpd_stop(server);
            
            ESP_LOGI("stop_http_server","ha pasado la funcion httpd_stop");
    
            if (err != ESP_OK) {
                ESP_LOGE("HTTP", "Error al detener el servidor HTTP: %s", esp_err_to_name(err));
            } else {
                ESP_LOGI("HTTP", "Servidor HTTP detenido correctamente.");
            }
            vTaskDelay(pdMS_TO_TICKS(500));  // Esperar para liberar completamente los recursos
            server = NULL;
            ESP_LOGI("stop_http_server", "Liberando mutex...");
            xSemaphoreGive(nvs_mutex);
            ESP_LOGI("stop_http_server", "mutex liberado");
        } else {
            ESP_LOGW("stop_http_server", "No se pudo obtener el mutex en el tiempo límite, abortando detención del servidor.");
        }
    } else {
        ESP_LOGW("HTTP", "Intentando detener servidor HTTP, pero no estaba en ejecución.");
    }
}
/*
void stop_server_task(void *arg) {
    while (1) {
        int signal;
        if (xQueueReceive(stop_server_queue, &signal, portMAX_DELAY)) {
            stop_http_server();
        }
    }
}*/


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
     * @brief inicia el servidor HTTP con la configuracion especificada
     * (config) y asigna el manejador al puntero (server)
     * 
     * En caso que la función httpd_start() tenga exito devuelve ESP_Ok y se cumple
     * la condición del if.
     * 
     * En caso que ocurra un error el condicional no se cumplirá y 
     * se imprimirá un mensaje de error en la terminal a traves de ESO_LOG()
     */

    esp_err_t ret = httpd_start(&server, &config);
    if (ret == ESP_OK) {
        ESP_LOGI("httpd_start", "servidor iniciado correctamente");
        
        //registrar ruta principal "/"
        if(httpd_register_uri_handler(server, &root)==ESP_OK){
            ESP_LOGI("start_http_server", "Manejador de ruta raiz registrado correctamente");
        }else{
            ESP_LOGE("start_http_server", "Error al registrar el manejador de ruta raiz");
        }

        // registrar ruta submit
        if (httpd_register_uri_handler(server, &submit)==ESP_OK)
        {
            ESP_LOGI("start_http_server", "Manejador de ruta submit registrada correctamente");
        }else
        {
            ESP_LOGI("start_http_server", "error al registrar la ruta submit");
        }
        
        // Registtrar rutas no definidas
        if (httpd_register_uri_handler(server, &redirect)==ESP_OK)
        {
            ESP_LOGI("start_http_server", "Manejador de rutas desconocidas registrado correctamente");
        }else
        {
            ESP_LOGI("start_http_server", "error al registrar la rutas desconocidas");
        }

        // Registrar ruta del archivo script
        if (httpd_register_uri_handler(server, &script)==ESP_OK)
        {
            ESP_LOGI("start_http_server", "Manejador de archivo javascript registrado correctamente");
        }else
        {
            ESP_LOGI("start_http_server", "error al registrar la ruta del archivo javascript");
        }

        //registrar ruta scann
        if (httpd_register_uri_handler(server, &scan)==ESP_OK)
        {
            ESP_LOGI("start_http_server", "Manejador de ruta scan registrada correctamente");
        }else
        {
            ESP_LOGE("start_http_server", "error al registrar la ruta scan");
        }

    } else if (ret == ESP_ERR_HTTPD_ALLOC_MEM) {
        ESP_LOGE("start_http_server", "Error al reservar memoria para el servidor HTTP");
    } else if (ret == ESP_ERR_HTTPD_TASK) {
        /**
         * en caso de que el servidor no se haya iniciado se imprime el
         * error en el monitor serie.
         */
        ESP_LOGE("start_http_server", "Error al iniciar el servidor HTTP");
    } else if (ret == ESP_ERR_INVALID_ARG){
        ESP_LOGE("start_http_server", "Argumentos invalidos para iniciar httpd");
    }
    else {
        ESP_LOGE("start_http_server", "Error: %s", esp_err_to_name(ret));
    }
}
void wifi_set_AP(void) {
    ESP_LOGI("wifi_set_AP", "Iniciando Wi-Fi en modo AP...");

    if (!ap_netif) {
        ap_netif = esp_netif_create_default_wifi_ap();
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
}

/**
 * @brief Establecer en modo AP+STA
 * 
 * Configura la ESP32 oara funcionar en modo Wi-Fi AP+STA permitiendo:
 * 1) Hacer que otros dispositivos puedan conectarse a la ESP32
 * 2) Actuar como un cliente en una red
 * 
 * Esta funcion tiene la particularidad de manejar correctamente los
 * cambios de modo Wi-Fi asi como reiniciar las interfaces de red en
 * caso de ser necesario.
 * 
 * @return Nada
 */
void wifi_set_AP_STA(void) {

    /**
     * Se imprime en el monitor serie un mensaje informativo indicando que el
     * wifi esta siendo configurado en modo AP+STA.
     */
    ESP_LOGI("wifi_set_AP_STA", "Iniciando Wi-Fi en modo AP+STA...");

    /**
     * Se establece un condicional que evalue el estado actual del Wi-Fi
     * mediante la función esp_wifi_get_mode() la cual devuelve el estado
     * en la variable "current_mode".
     * 
     * de esta forma, si la ESP32 estaba en modo STA cuando se invoco esta
     * función, Se detiene la conexion actual con esp_wifi_stop() y se
     * agrega un pequeño retardo con vTaskDelay() para evitar conflictos
     * en la reconfiguracion.
     * 
     * en caso que no se pueda obtener el estado se imprime una advertencia
     * pero se sigue con la configuración. esto es util si la ESP32 no habia
     * creado una interfaz previa.
     */
    wifi_mode_t current_mode;
    if (esp_wifi_get_mode(&current_mode) == ESP_OK) {
        // **Si está en modo STA, detener Wi-Fi antes de cambiar a AP+STA**
        if (current_mode == WIFI_MODE_STA) {
            ESP_LOGI("wifi_set_AP_STA", "Wi-Fi estaba en modo STA, deteniéndolo antes de cambiar a AP+STA...");
            ESP_ERROR_CHECK(esp_wifi_stop());
            ESP_LOGI("wifi_set_AP_STA", "modo STA detenido");
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    } else {
        ESP_LOGW("wifi_set_AP_STA", "No se pudo obtener el modo Wi-Fi. Continuando con la configuración...");
    }

    /**
     * Con la función esp_wifi_set_mode() se establece que el Wi-Fi cambie a 
     * modo AP+STA usando la macro WIFI_MODE_APSTA
     */
    ESP_LOGI("wifi_set_AP_STA", "Estableciendo modo AP+STA ...");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_LOGI("wifi_set_AP_STA", "Modo AP+STA establecido");
    
    /**
     * En el caso de que exista alguna interfaz de red previa (AP o STA), esta 
     * se destruye para prevenir fugas de memoria y conflictos de reconfiguracón.
     */
    if (ap_netif) {
        esp_netif_destroy(ap_netif);
        ap_netif = NULL;
    }
    if (sta_netif) {
        esp_netif_destroy(sta_netif);
        sta_netif = NULL;
    }

    /**
     * Se crean las interfaces AP y STA usando las funciones 
     * esp_netif_create_...()
     */
    ap_netif = esp_netif_create_default_wifi_ap();
    sta_netif = esp_netif_create_default_wifi_sta();
    if (ap_netif && sta_netif)
    {
        ESP_LOGI("wifi_set_AP_STA", "Interfaces AP y STA creadas correctamente");
    }
    if (!ap_netif)
    {
        ESP_LOGE("wifi_set_AP_STA", "Error al crear la interfaz AP");
    }
    if (!sta_netif)
    {
        ESP_LOGE("wifi_set_AP_STA", "Error al crear la interfaz STA");
    }
    
    
    
    
    /**
     * Se arma la estructura de configuración del AP. esta estructura contiene
     * los siguientes parametros:
     * 
     * @param[in] ssid Nombre del wifi creado por la ESP32
     * @param[in] password contraseña de la red AP
     * @param[in] ssid_len longitud del ssid
     * @param[in] channel canal Wi-Fi en el que operará el AP
     * @param[in] max_connection máximo numero de dispositivos que pueden conectarse
     * @param[in] authmode modo de autenticación (WPA/WPA2)
     */
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .password = AP_PASSWORD,
            .ssid_len = strlen(AP_SSID),
            .channel = 3,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    /**
     * En caso de que la cadena de la contraseña este vacia, se cambia el authmode en 
     * "modo abierto" donde el cliente no debe autenticarse con contraseña.
     */
    if (strlen(AP_PASSWORD) == 0) {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    /**
     * Se configura la ESP32 como punto de acceso con los parametros 
     * definidos en wifi_conf
     */
    ESP_LOGI("wifi_set_AP_STA", "estableciendo configuracion de punto de acceso ...");
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    //Se inicia el Wi-Fi con la nueva configuración (modo AP+STA)
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI("wifi_set_AP_STA", "Punto de acceso iniciado correctamente");

    //Se imprime el SSID del AP en el monitor serie
    ESP_LOGI("wifi_set_AP_STA", "interfaz de configuración activa con SSID: %s", AP_SSID);
    
    //Se comprueba el estado del Wi-Fi y se imprime en monitor serie
    esp_wifi_get_mode(&current_mode);
    ESP_LOGI("wifi_set_AP_STA", "Modo Wi-Fi actual: %d", current_mode);
    
    /**
     * Se enciende el LED RGB en color Cyan indicando que el portal cautivo
     * está activo.
     */
    set_led_color(COLOR_CYAN);
}

/**
 * @brief Inicializacion del sistema wifi
 * 
 * esta funcion se encarga de asegurar que la memoria no volatil y los eventos
 * del sistema estén correctamente configurados.
 * 
 * es necesario llamarla al inicio de un programa que usa wifi.
 * 
 * para mas detalles dirijase a:
 * 
 * https://github.com/victor456472/carpetas-de-prueba-esp32-c6-devkitc-1/tree/master/18-portal-cautivo-conexion-wifi-V1.3#21-inicializar-sistema-wifi
 * 
 * @return nada
 */
void wifi_system_init(void){
    /**
     * Inicializa la memoria no volatil donde se almacenan las configuraciones
     * persistentes como las credenciales Wi-Fi
     * 
     * @return retorna un codigo de error que indica si la inicialización fue 
     * exitosa o falló
     */
    ESP_LOGI("wifi_system_init", "Iniciando memoria NVS...");
    esp_err_t ret = nvs_flash_init();
    /**
     * Si la inicialización de NVS falla por uno de estos dos motivos:
     * 
     * - ESP_ERR_NVS_NO_FREE_PAGES: No hay más espacio en la memoria NVS.
     * - ESP_ERR_NVS_NEW_VERSION_FOUND: Se detecta una versión diferente de NVS 
     * (por ejemplo, tras una actualización del firmware).
     * 
     * Esto se soluciona borrando y reinicializando la memoria NVS
     */
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Borra toda la memoria NVS
        ESP_LOGI("wifi_system_init", "error, borrando la memoria NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_LOGI("wifi_system_init", "Memoria NVS borrada");
        // vuelve a inicializar NVS desde cero
        ESP_LOGI("wifi_system_init", "Volviendo a iniciar memoria NVS...");
        ESP_ERROR_CHECK(nvs_flash_init());
        ESP_LOGI("wifi_system_init", "Memoria NVS iniciada correctamente");
    }else{
        ESP_LOGI("wifi_system_init", "Memoria NVS iniciada correctamente");
    }
    /**
     * Inicializa el sistema de redes en la ESP32. Esto es necesario antes de
     * configurar cualquier interfaz Wi-Fi o Ethernet
     */
    ESP_LOGI("wifi_system_init", "Iniciando sistema de redes NETIF...");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_LOGI("wifi_system_init", "sistema de redes NETIF iniciado");
    /**
     * Crea el bucle de eventos principal de la ESP32, que se encargará de manejar
     * eventos de Wi-Fi, red y otros perifericos
     */
    ESP_LOGI("wifi_system_init", "Creando bucle de eventos principal...");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI("wifi_system_init", "bucle de eventos principal creado correctamente");

    /**
     * - wifi_init_config_t es una estructura que contiene parámetros de 
     *   configuración para Wi-Fi.
     * 
     * - WIFI_INIT_CONFIG_DEFAULT() es un macro que devuelve una configuración 
     *   estándar recomendada por ESP-IDF.
     */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    //inicializa el controlador Wi-Fi con la configuración establecida.
    ESP_LOGI("wifi_system_init", "Iniciando controlador Wi-Fi");
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_LOGI("wifi_system_init", "Controlador Wi-Fi Iniciado correctamente");

}

/**
 * @brief Esta fuunción define una tarea que monitorea un boton conectado al GPIO 18 de la ESP32
 * y realiza una acción especifica si el botón es mantenido presionado durante un tiempo prolongado
 * (en este caso 10 segundos).
 * 
 * La accion consiste en borrar las credenciales Wi-Fi almacenadas en la memoria no volatil (NVS),
 * reinicializarla y volcer a configurar el dispositivo en modo Access Point
 */
void reset_handler(void *param) {
    /**
     * Esta variable almacena el momento en que el botón fue presionado, en milisegundos. Sí es 
     * 0 significa que el botón no está siendo presionaado actualmente.
     */
    uint64_t press_start_time = 0;

    /**
     * Esta variable indica si el botón ha sido mantenido presionado durante mas de un tiempo
     * específico (HOLD_TIME_MS). Esto asegura que la acción de borrar la memoria NVS se realice
     * solo una vez mientras el botón está presionado.
     */  
    bool button_held = false;       // Estado del botón (si fue mantenido por más de 10 segundos)

    //bucle

    /**
     * El bucle tiene un retardo de 10 ms entre cada iteración para reducir el consumo de CPU.
     */
    while (1) { 
        // Leer el estado del botón. 1 si el botón está presionado, 0 si no está presionado.
        int button_state = gpio_get_level(BUTTON_GPIO);

        /**
         * Se evalua si el botón es presionado
         */
        if (button_state == 1) {  
            /**
             * en el caso de ser presionado, si es la primera vez que se detecta la presión
             * (press_start_time == 0) se registra el tiempo actual (en milisegundos) usando 
             * esp_timer_get_time(), que retorna el tiempo en microsegundos desde que el ESP32
             * arrancó (similar a millis() en arduino pero en microsegundos)
             */
            if (press_start_time == 0) {
                // Registrar el momento en que se presionó el botón
                press_start_time = esp_timer_get_time() / 1000; // Tiempo actual en ms
            } 
            /**
             * En caso que no sea la primera vez que se detecta la presión
             */
            else {
                /**
                 * Se calcula cuanto tiempo ha transcurrido desde que se presionó el botón 
                 */
                uint64_t elapsed_time = (esp_timer_get_time() / 1000) - press_start_time;
                /**
                 * Se evalua si el tiempo transcurrido es mayor o igual a HOLD_TIME_MS (en este
                 * caso 10000ms) y si la acción no se ha realizado aún (!button_held)
                 */
                if (elapsed_time >= HOLD_TIME_MS && !button_held) {
                    
                    //En caso verdadero se marca el botón como "mantenido" (button_held=true)
                    button_held = true;

                    ESP_LOGI("reset_handler", "invocando detencion de eventos IP...");
                    if (stop_IP_events() == ESP_OK) {
                        ESP_LOGI("reset_handler", "Eventos IP desregistrados correctamente.");
                    } else {
                        ESP_LOGW("reset_handler", "Los eventos IP no estaban registrados o hubo un error.");
                    }                    
                    if (stop_WIFI_events() == ESP_OK) {
                        ESP_LOGI("reset_handler", "Eventos WIFI desregistrados correctamente.");
                    } else {
                        ESP_LOGW("reset_handler", "Los eventos WIFI no estaban registrados o hubo un error.");
                    }                    
                    //Se imprime el mensaje "borrando toda la memoria NVS" en el monitor serie
                    ESP_LOGI("reset_handler", "Borrando toda la memoria NVS...");
                    clear_wifi_credentials_from_nvs();
                    ESP_LOGI("reset_handler", "Las credenciales Wi-Fi han sido eliminadas");

                    // Se detiene la lectura del ADC
                    ESP_ERROR_CHECK(stop_timer_adc());
                    // Se coloca la ESP32 en modo AP
                    wifi_set_AP_STA();
                    wifi_connected = false;
                    // deshabilitar ahorro de batería
                    ESP_LOGI("reset_handler", "deshabilitando ahorro de energía...");
                    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
                    ESP_LOGI("reset_handler", "ahorro de energia deshabilitado");
                    // Se conecta el servidor http
                    // start_http_server();
                }
            }
        /**
         * Si el botón no es presionado o se suelta
         */
        } else {  
            press_start_time = 0; // Reiniciar el tiempo de inicio
            button_held = false;  // Reiniciar el estado del botón mantenido
        }

        // Espera 10ms para reducir el consumo de CPU
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }
}


/**
 * @brief Lectura del sensor
 * 
 * vTimerCallback() se ejecuta automáticamente cada vez que el temporizador
 * expira, es decir, cada SAMPLE_PERIOD_MS (milisegundos).
 * 
 * Su función principal es leer el valor del ADC y registrarlo en el monitor
 * serie.
 * 
 * @param[in] pxTimer Este es un manejador del temporizador freeRTOS
 * en esta ocación no es usado pero podria servir para obtener información 
 * del temporizador si hubiera varios activos.
 * 
 * @return nada.
 */
void vTimerCallback(TimerHandle_t pxTimer)
{
    // inicializa adc_val en 0 antes de realizar la lectura
    adc_val = 0;

    /**
     * Se lee el valor del canal ADC especificado.
     * 
     * @param[in] adc_handle manejador de la unidad ADC (inicializado en set_adc()).
     * @param[in] ADC_CHANNEL canal ADC especifico donde esta conectado el sensor
     * @param[out] adc_val direccion de memoria donde se almacenará el resultado.
     * 
     * @return Devuelve un código de error (esp_err_t ret), indicando si la lectura 
     * fue exitosa (ESP_OK) o falló.
     */
    esp_err_t ret = adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_val);
    
    /**
     * Se verifica si la lectura fue exitosa. para ello se emplea la variable ret
     * la cual toma el valor de ESP_OK en caso positivo.
     * 
     * Si la operacion es exitosa se imprime la lectura del sensor en el monitor
     * serie. Por lo contrario si la lectura falla se imprime el mensaje de error
     * en el monitor serie
     */
    if (ret == ESP_OK)
    {
        ESP_LOGI("vTimerCallback", "Lectura del sensor: %d", adc_val);
    }
    else
    {
        ESP_LOGE("vTimerCallback", "Error al leer el ADC: %s", esp_err_to_name(ret));
    }
}

/**
 * @brief código principal
 * 
 * La función app_main() es el punto de entrada del programa en ESP-IDF y se 
 * encarga de inicializar y configurar todos los componentes esenciales de la ESP32 
 * para manejar la conectividad Wi-Fi, el ADC, los GPIOs y el servidor HTTP.
 * 
 * Este código configura la ESP32 para operar en modo estación (STA) si ya tiene 
 * credenciales Wi-Fi almacenadas, o en modo punto de acceso (AP) si no se han 
 * guardado credenciales. También gestiona sensores, GPIOs y eventos Wi-Fi.
 */
void app_main(void) {

    nvs_mutex = xSemaphoreCreateMutex();

    wifi_system_init();

    //inicializar gpios
    init_gpio();

    //inicializar controles del adc
    set_adc();
    set_timer_adc();

    /**
     * Se crea una tarea en FreeRTOS que revisa si se debe limpiar las credenciales 
     * Wi-Fi almacenadas en la memoria NVS.
     * 
     * Esta tarea se ejecuta en paralelo y está ligada a un botón de reset que, 
     * si se mantiene presionado durante cierto tiempo, borra las credenciales Wi-Fi.
     */
    xTaskCreate(reset_handler, "reset_handler", 2048, NULL, 3, NULL);

    //stop_server_queue = xQueueCreate(1, sizeof(int));  // Crear cola de tamaño 1
    //xTaskCreate(stop_server_task, "stop_server_task", 4096, NULL, 2, NULL);
    // Inicializar LED RGB del ESP32
    init_led_strip();
    vTaskDelay(pdMS_TO_TICKS(200));
    set_led_color(COLOR_ORANGE);
    vTaskDelay(pdMS_TO_TICKS(10000));

    /**
     * Se crean variables para almacenar el SSID y la contraseña de la red Wi-Fi 
     * a la que la ESP32 intentará conectarse.
     * 
     * El tamaño del SSID es de hasta 32 caracteres y la contraseña hasta 64 caracteres.
     */
    char ssid[32] = {0};
    char password[64] = {0};

    /**
     * Se intenta leer las credenciales de conexión Wi-Fi almacenadas en la memoria NVS.
     * Si existen credenciales guardadas, se almacenan en ssid y password.
     * 
     * Si la ESP32 se ha conectado antes a una red, estas credenciales estarán disponibles 
     * y evitarán que el usuario tenga que ingresarlas manualmente.
     */
    wifi_mode_t mode = read_wifi_config_from_nvs(ssid, sizeof(ssid), password, sizeof(password));

    /**
     * Se verifica si el Wi-Fi está configurado en modo STA y si las credenciales no 
     * están vacías. Si las credenciales existen, la ESP32 se conecta automáticamente a la 
     * red Wi-Fi.
     * 
     * Si no hay credenciales, la ESP32 se iniciará en modo AP para que el usuario pueda 
     * configurar la conexión por medio de un portal cautivo.
     */
    if (mode == WIFI_MODE_STA && strlen(ssid) > 0 && strlen(password) > 0) {
        // Si hay credenciales, iniciar en modo STA
        start_WIFI_events();
        wifi_set_STA();
        wifi_connect_STA(ssid, password);
    } else {
        // Si no hay credenciales, iniciar en modo AP
        init_spiffs();
        wifi_set_AP_STA();
        // Deshabilitar el ahorro de energía para un escaneo más preciso
        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
        start_http_server();
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
        ESP_LOGE("root_handler", "No se pudo abrir el archivo HTML");
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

/**
 * @brief Este es un manejador HTTP que procesa una solicitud enviada desde un formulario
 * web al servidor alojado en la ESP32. Su proposito principale es extraer las credenciales
 * Wi-Fi (SSID y contraseña) del formulario, decodificarlas, almacenarlas en la memoria no
 * volatil (NVS) y cambiar la ESP32 al modo estación (STA) para conectarse a una red Wi-Fi
 * 
 * @param[in] req puntero a la estructura httpd_req_t que representa la solicitud HTTP
 * recibida. Esta estructura contiene informacion relevante como los parametros enviados
 * desde el formulario web.
 * 
 * @return Devuelve un codigo de error o éxito. ESP_OK indica que la función se ejecutó
 * correctamente.
 */
esp_err_t submit_handler(httpd_req_t *req) {
    // se declara un buffer de 100 bytes para extraer la cadena de consulta de la URL
    char param[100];
    /**
     * La función httpd_req_get_url_query_str() permite recibir el query como una cadena
     * de caracteres. tiene como parametros:
     * 
     * 1- estructura de la solicitud http recibida.
     * 2- buffer donde se almacenará la cadena de caracteres, en este caso es la variable
     * param.
     * 3- tamaño del buffer donde se almacenará ña cadena de caracteres.
     * 
     * Si la función se ejecuta correctamente devuelve ESP_OK lo que significa que hay datos
     * que procesar. por el contrario se debe devolver un mensaje de error al cliente.
     */
    if (httpd_req_get_url_query_str(req, param, sizeof(param)) == ESP_OK) {
        /**
         * Los buffers ssid_encoded y password_encoded sirven para almacenar el SSID y la
         * contraseña tal como se reciben en formato codificado (URL encoding)
         */
        char ssid_encoded[50];
        char password_encoded[50];
        /**
         * Los buffers ssid y password se usan para almacenar los valores decodificados
         * despues de eliminar el URL encoding
         */
        char ssid[50];
        char password[50];

        /**
         * Se buscan los parametros especificos en la cadena param y se almacenan sus 
         * valores en los buffers ssid_encoded y password_encoded. la condicion indica que
         * deben recibirse ambos parametros para entrar en "verdadero".
         * 
         * Es importante aclarar que la función httpd_query_key_value permite buscar 
         * en la cadena de consulta el parametro especificado y almacenarlo en un buffer 
         * para su posterior uso. Si la operación es correcta devuelve ESP_OK.
         */

        if (httpd_query_key_value(param, "SSID", ssid_encoded, sizeof(ssid_encoded)) == ESP_OK &&
            httpd_query_key_value(param, "PASSWORD", password_encoded, sizeof(password_encoded)) == ESP_OK) {

            // Decodificar SSID y contraseña
            url_decode(ssid_encoded, ssid, sizeof(ssid));
            url_decode(password_encoded, password, sizeof(password));

            // Se imprimen las credenciales decodificadas en el monitor serial
            ESP_LOGI("submit_handler", "SSID Decodificado: %s", ssid);
            ESP_LOGI("submit_handler", "PASSWORD Decodificado: %s", password);

            // **Enviar respuesta indicando que se intentará conectar**
            httpd_resp_set_type(req, "text/plain");
            httpd_resp_send(req, "Intentando conectar a la red Wi-Fi...", strlen("Intentando conectar a la red Wi-Fi..."));

            // **Ahora iniciamos la conexión después de enviar la respuesta**
            vTaskDelay(pdMS_TO_TICKS(1000));  // Pequeña espera para evitar interferencias
            ESP_LOGI("submit_handler", "Memoria libre antes de conectar: %lu bytes", (unsigned long)esp_get_free_heap_size());
            ESP_LOGI("submit_handler", "SSID: %s, Password: %s", ssid, password);
            esp_err_t connection_state = wifi_connect_STA(ssid, password);
            if (connection_state == ESP_OK) {
                ESP_LOGI("submit_handler", "Conexión establecida correctamente.");
                
                //int stop_signal = 1;
                //xQueueSend(stop_server_queue, &stop_signal, portMAX_DELAY);
                //vTaskDelay(pdMS_TO_TICKS(500));
                if(stop_WIFI_events()==ESP_OK){
                    ESP_LOGI("submit_handler", "Eventos wifi detenidos correctamente");
                }else
                {
                    ESP_LOGE("submit_handler","Error: no se pudo detener los eventos WIFI");
                }

                if(stop_IP_events()==ESP_OK){
                    ESP_LOGI("submit_handler", "Eventos ip detenidos correctamente");
                }else
                {
                    ESP_LOGE("submit_handler","Error: no se pudo detener los eventos IP");
                }
                
                wifi_set_STA();
                start_WIFI_events();
                start_IP_events();

                save_wifi_config_to_nvs(WIFI_MODE_STA, ssid, password);
                ESP_ERROR_CHECK(esp_wifi_connect());
                wifi_mode_t mode_test = read_wifi_config_from_nvs(ssid, sizeof(ssid), password, sizeof(password));
                ESP_LOGI("submit_handler", "Credenciales en NVS: mode = %d, SSID = %s, PASSWORD = %s", mode_test, ssid, password);

                //se habilita el ahorro de bateria
                ESP_LOGI("submit_handler", "Habilitando ahorro de energía Wi-Fi...");
                ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
                ESP_LOGI("submit_handler", "ahorro de energia habilitado");
            } 
            else {
                ESP_LOGE("submit_handler", "Error desconocido al conectarse. verifique que las credenciales sean correctas y que la red este activa");
                for (size_t i = 0; i < 3; i++)
                {
                    set_led_color(COLOR_MAGENTA);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    set_led_color(TURN_OFF);
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
                set_led_color(COLOR_CYAN);
                const char redirect_html[] = "<html><head>"
                             "<meta http-equiv='refresh' content='0; url=/' />"
                             "<script>window.location.href='/';</script>"
                             "</head></html>";
                httpd_resp_set_type(req, "text/html");
                httpd_resp_send(req, redirect_html, strlen(redirect_html));

            }

            return ESP_OK;
        } else {
            httpd_resp_sendstr(req, "Error al procesar los datos del formulario.");
        }
    } else {
        httpd_resp_sendstr(req, "Error: No se recibieron parámetros.");
    }
    /**
     * Se retorna ESP_OK si la operacion es exitosa.
     */
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

esp_err_t script_handler(httpd_req_t *req) {
    FILE *file = fopen("/spiffs/script.js", "r");

    if (!file) {
        ESP_LOGE("script_handler", "No se pudo abrir el archivo script.js");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/javascript");

    char buffer[1024];
    size_t read_bytes;

    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        httpd_resp_send_chunk(req, buffer, read_bytes);
    }

    httpd_resp_send_chunk(req, NULL, 0);
    fclose(file);
    return ESP_OK;
}

esp_err_t scan_handler(httpd_req_t *req) {

    ESP_LOGI("scan_handler", "Ejecutando escaneo de redes...");

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };

    ESP_LOGI("scan_handler", "Iniciando escaneo de redes...");
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true)); // Realizar escaneo sincrónico

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    ESP_LOGI("scan_handler", "Número de redes encontradas: %d", ap_count);

    if (ap_count == 0) {
        ESP_LOGW("scan_handler", "No se encontraron redes Wi-Fi.");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "[]", strlen("[]")); // Enviar lista vacía al cliente
        return ESP_OK;
    }

    // **⚠ Reservar memoria dinámicamente para evitar stack overflow ⚠**
    wifi_ap_record_t *ap_records = (wifi_ap_record_t *)malloc(ap_count * sizeof(wifi_ap_record_t));
    if (!ap_records) {
        ESP_LOGE("scan_handler", "Error: No se pudo asignar memoria para los registros de redes.");
        httpd_resp_send_500(req); // Responder con error 500
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));

    char json_response[1024] = "[";
    for (int i = 0; i < ap_count; i++) {
        ESP_LOGI("scan_handler", "SSID detectado: %s", ap_records[i].ssid);
        char ssid_entry[128];
        snprintf(ssid_entry, sizeof(ssid_entry), "\"%s\"%s", ap_records[i].ssid, (i < ap_count - 1) ? "," : "");
        strcat(json_response, ssid_entry);
    }
    strcat(json_response, "]");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, strlen(json_response));

    // Liberar memoria despues de usarla|
    free(ap_records); 

    return ESP_OK;
}

/**
 * @brief Establecer temporizador del ADC
 * 
 * La función set_timer_adc() configura un temporizador en FreeRTOS 
 * (xTimerCreate) para ejecutar una tarea de lectura del ADC de manera
 * periodica.
 * 
 * @return
 *      - ESP_OK: Si la operacion es ejecutada exitosamente
 *      - ESP_FAIL: Si ocurre un error creando el timer
 */
esp_err_t set_timer_adc(void)
{
    /**
     * Se crea el handler del timer freeRTOS usando xTimerCreate(). 
     * Este tiene como parametros:
     * 
     * @param[in] pcTimerName nombre del timer
     * @param[in] xTimerPeriodInTicks periodo del timer en TICKS (en este 
     * caso se establece en un numero equivalente a 100ms)
     * @param[in] xAutoReload modo periodico (En este caso está habilitado)
     * @param[in] pvTimerID ID del temporizador (no se usa en este caso)
     * @param[out] pxCallbackFunction Función de callback que se ejecuta al
     * activarse el timer.
     * 
     * @return retorna el handler del timer que se acaba de crear
     * 
     */
    ESP_LOGI("set_timer_adc", "Configurando prioridad del Timer Service Task...");
    vTaskPrioritySet(xTimerGetTimerDaemonTaskHandle(), 2);
    xTimerADC = xTimerCreate(
        "Timer",
        (pdMS_TO_TICKS(SAMPLE_PERIOD_MS)),
        pdTRUE,
        (void *)0,
        vTimerCallback);
    
    UBaseType_t priority = uxTaskPriorityGet(xTimerGetTimerDaemonTaskHandle());
    ESP_LOGI("set_timer_adc", "Prioridad actual del Timer Service Task: %d", priority);

    /**
     * En el caso que no se haya creado correctamente el handler del timer 
     * se retorna ESP_FAIL y se imprime el mensaje "error creando el timer."
     */
    if (xTimerADC == NULL)
    {
        ESP_LOGE("set_timer_adc", "Error creando el timer.");
        return ESP_FAIL;
    }

    // Si la operación fue exitosa se retorna ESP_OK
    return ESP_OK;
}

esp_err_t start_timer_adc()
{
    if (xTimerADC == NULL)
    {
        ESP_LOGE("start_timer_adc", "El timer no ha sido inicializado.");
        return ESP_FAIL;
    }

    if (xTimerStart(xTimerADC, 0) != pdPASS)
    {
        ESP_LOGE("start_timer_adc", "Error al iniciar el timer.");
        return ESP_FAIL;
    }
    ESP_LOGI("start_timer_adc", "Timer iniciado.");
    return ESP_OK;
}

esp_err_t stop_timer_adc()
{
    if (xTimerADC == NULL)
    {
        ESP_LOGE("stop_timer_adc", "El timer no ha sido inicializado.");
        return ESP_FAIL;
    }
    if (xTimerStop(xTimerADC, 0) != pdPASS)
    {
        ESP_LOGE("stop_timer_adc", "Error al detener el timer.");
        return ESP_FAIL;
    }
    ESP_LOGI("TAG_ADC", "Timer detenido.");
    return ESP_OK;
}

/**
 * @brief configurar ADC
 * 
 * la función set_adc() configura y habilita un ADC (Conversor Analógico-Digital) en la ESP32 
 * utilizando el modo de lectura "oneshot", que permite realizar mediciones de voltaje bajo 
 * demanda a traves del GPIO 5.
 * 
 * @return Nada
 */
static void set_adc(void)
{
    /**
     * @brief configuracion de la unidad ADC
     * 
     * La estructura adc_oneshot_unit_init_cfg_t permite configurar el modulo
     * ADC. los campos principales de esta estructura son:
     * 
     * @param[in] unit_id Selecciona que unidad de ADC se usará (El ESP32-C6 tiene
     * ADC_UNIT:1 y ADC_UNIT_2). para este caso se selecciona la unidad 1
     * 
     * @param[in] ulp_mode Confirma si se desea activar o desactivar el modo ULP.
     * En este caso se desactiva ya que se quieren mediciones normales.
     */
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    /**
     * Se inicializa la unidad del ADC con la configuracion de init_config mediante 
     * la función adc_oneshot_new_unit(). los parametros de esta funcion son:
     * 
     * @param[in] init_config driver de configuraciones. se requiere la direccion
     * de memoria de la estructura adc_oneshot_unit_init_cfg_t
     * 
     * @param[out] ret_unit manejador de la unidad del ADC el cual se aloja en la
     * dirección de memoria del handler del ADC. En este se guardaran las configuraciones 
     * de la unidad del ADC y el estado del modo ULP.
     */
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    /**
     * @brief Configuración del canal ADC
     * 
     * la estructura adc_oneshot_chan_cfg_t permite configurar el canda del ADC
     * sobre el cual se realizará la lectura. los parametros del canal son:
     * 
     * @param[in] bitwidth Configura la resolución en bits del ADC. en este caso
     * se establece una resolución de 12 bits para una lectura de entre 0 y 4096
     * @param[in] atten configura la atenuación del voltaje de entrada. esto es
     * necesario ya que el ADC del ESP32-C6 funciona con un voltaje maximo de 1.1
     * voltios y normalmente los DevKits manejan voltajes entre 3.3 y 5 voltios.
     * en este caso se decidió usar una atenuación de 6dB para una entrada de
     * voltaje de 2.2 voltios para asegurar todo el campo de medida del ADC.
     */
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTENUATION};

    /**
     * Se aplica la configuracion al canal con los parametros declarados en la
     * estructura config. la funcion adc_oneshot_config_channel tiene los siguientes
     * parametros:
     * 
     * @param[in] handle handler del ADC
     * @param[in] chan canal del adc. En este caso se usa el canal 5, el cual se 
     * aloja en el GPIO 5
     * @param[in] config estructura de configuración adc_oneshot_chan_cfg_t
     * 
     * @return 
     *      - ESP_OK: Si la operación se realiza correctamente
     *      - ESP_ERR_INVALID_ARG: Si se coloca algun argumento invalido
     *      - ESP_ERR_TIMEOUT: Si el sistema demora mucho tiempo en configurar 
     *        el canal por algun motivo.
     */
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));
}
