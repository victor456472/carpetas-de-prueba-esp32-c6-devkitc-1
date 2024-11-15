/*
Esta practica consiste en escanear los access point de la red wifi.

el desarrollo del codigo se basó en el tutorial que se muestra a continuacion:

https://youtube.com/playlist?list=PLmQ7GYcMY-2JV7afZ4hiekn8D6rRIgYfj&si=cQHAoQEpCTzz2vWw

video #11

sin embargo, para el caso del esp32-c6-DevKitC-1 habia que actualizar algunos metodos
que quedaron obsoletos desde la fecha de la publicación. algunos de ellos son los
pertenencientes a esp_event_loop.h que ahora migraron a esp_event.h

*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

/*
Esta funcion permite inicializar la conexion wifi del ESP32 invocando el bucle
de eventos, la memoria no volatil y las configuraciones de la conexion wifi.
*/

void wifi_init(void) {
    /*
    con el siguiente codigo se inicializa el almacenamiento no volatil NVS en el 
    ESP32 sobre el cual se van a guardar configuraciones que deben ser persistentes entre 
    reinicios como las configuraciones de Wi-Fi

    la variable ret va a retornar el estado de la operacion de inicializacion. en caso
    de haber un error este sera atrapado por esta variable y podra detectarse mediante un 
    condicional IF. si la operación es exitosa se devuelve ESP_OK
    */

    esp_err_t ret = nvs_flash_init(); 

    /*
    existen dos errores que pueden ocurrir durante la inicializacion de la memoria no
    volatil.

    #1

    uno de estos es cuando la memoria no volatil esta llena y no existen "paginas libres"
    en la memoria flash. Una "pagina" es una seccion de memoria flash de tamaño fijo
    que se puede leer, escribir o borrar como una unidad completa.

    la memoria no volatil se utiliza para almacenar pares clave-valor (como un diccionario
    en python) que contienen configuraciones, datos de usuario o cualquier dato que se 
    quiera mantener entre reinicios.

    puede ocurrir que todas las paginas de la memoria no volatil estén ocupadas al 
    momento de inicializar. En este caso nvs_flash_init() devuelve el codigo de error
    ESP_ERR_NVS_NO_FREE_PAGES.

    #2

    Otro error puede ocurrir cuando se encuentrea una version de memoria no volatil 
    diferente a la que se espera. esto puede suceder si se actualizó el firmware y 
    cambió la versión de NVS, lo cual puede causar incopatibilidades en el formato
    de almacenamiento 

    para solucionar estos dos errores se puede borrar el contenido de la memoria no 
    volatil usando nvs_flash_erase() y asi inicializar la memoria con todas las 
    paginas disponibles para almacenar datos.
    
    finalmente se verifica el estado de la inicializacion de la flash mediante la 
    variable ret. en caso de no devolver ESP_OK se detiene el programa y se captura
    el error.
    */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /*
    Se inicializa la interfaz de red (esp_netif), que es la capa de abstracción para
    gestionar las conexiones de red en el ESP 32 (recordar modelo OSI). la interfaz 
    de red se encarga de manejar aspectos de la conexión como las configuraciones de 
    Wi-Fi, Ethernet y otros protocolos de red que el dispositivo puede usar.

    en el caso que ocurra un error al inicializar la capa de abstracción, esta 
    devuelve un valor distinto a ESP_OK por lo cual la funcion ESP_ERROR_CHECK() 
    detecta el problema y detiene el programa
    */

    ESP_ERROR_CHECK(esp_netif_init()); // Inicialización de la interfaz de red

    /*
    crea un bucle de eventos predeterminado en el ESP32. Un bucle de eventos es una 
    estructura que permite manejar eventos de diferentes fuentes (como WiFi, Bluetooth),
    temporizadores, etc.) de manera centralizada.

    El bucle de eventos permite que el ESP32 maneje diversos eventos sin la necesidad
    de estar constantemente revisando ("preguntando") si algo ha cambiado. en lugar
    de eso, cada evento se "registra" en el bucle, y cuando ocurre, el sistema "avisa" 
    a las funciones correspondientes para que respondan.
    
    ¿Cómo funciona el bucle de eventos?

        • Registrar eventos: El programa puede registrar funciones (a veces llamadas 
        "manejadores" o "handlers") que serán llamadas cuando ocurra un evento específico. 
        Por ejemplo, puedes registrar una función que maneje el evento de "Wi-Fi conectado".

        • Generar eventos: Cuando algo significativo ocurre, el sistema genera un evento. 
        Por ejemplo, cuando el ESP32 se conecta exitosamente a una red Wi-Fi, el sistema 
        genera un evento llamado WIFI_EVENT_STA_CONNECTED.

        • Despacho de eventos: El bucle de eventos toma este evento y busca todas las 
        funciones que están registradas para responder a este tipo de evento. Luego llama 
        a cada función registrada, permitiéndole ejecutar el código necesario para manejar 
        el evento.

        • Respuesta al evento: Las funciones registradas pueden realizar acciones como 
        cambiar configuraciones, enviar datos, activar otras partes del programa, etc., 
        dependiendo del tipo de evento.
    */

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /*
    Se configura la interfaz de red para que funcione en modo estación (cliente),
    lo que permitirá que el dispositivo se conecte a una red Wi-Fi en lugar de 
    actuar como punto de acceso.
    */

    esp_netif_create_default_wifi_sta(); 

    /*
    la macro WIFI_INIT_CONFIG_DEFAULT() define valores predeterminados para varios
    parametros dentro de la estructura wifi_init_config_t. Estos parametros incluyen 
    configuraciones importantes para la operación del Wi-Fi. algunos de estos parametros
    son:

    1) osi_funcs: establece los punteros para que usen las funciones del sistema
    operativo FreeRTOS las cuales hacen la gestion de eventos.

    2) ampdu_tx_enable y ampdu_rx_enable: AMPDU (Aggregate MAC Protocol Data Unit)
    es una técnica que permite enviar múltiples paquetes en una sola transmisión para
    mejorar el rendimiento de la red.

    3) nvs_enable: Activa el almacenamiento NVS para guardar las credenciales de 
    Wi-Fi, como el SSID y la contraseña.

    4) sta_init_parm y ap_init_parm: son configuraciones de parametros para el modo 
    estación (cliente) y el modo de punto de acceso, respectivamente.

    5) magic: Es un numero unico que permite validar la estructura de configuración.
    esto ayuda a detectar si la configuracion ha sido corrompida.
    */

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
}

void wifi_scan(void) {

    /*
    wifi_scan_config_t es una estructura que define los parametros para el escaneo 
    de redes wifi. al establecer ssid y bssid en NULLL se está especificando que
    se escanearan todas las redes sin filtrar por un SSID especifico (nombre de red)
    ni por un BSSID especifico (direccion MAC del punto de acceso).

    cuando se establece channel = 0 se indica que se escaneen todos los canales de la 
    banda de frecuencias en las que opera el protocolo de Wi-Fi

    listado de canales segun la frecuencia en GHz:

    https://es.wikipedia.org/wiki/IEEE_802.11

    finalmente, al establecer "show_hidden" en true se indica que se muestren tambien 
    las redes ocultas.

    */
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true // Escanea incluso redes ocultas
    };

    /*
    con el comando esp_wifi_scan_start() se inicia el escaneo de redes WiFi. este tiene
    dos parametros:

    1) la configuracion definida en scan_config
    2) este segundo parametro indica que el escaneo sera bloqueante, es decir, el
    programa esperará a que el escaneo termine antes de continuar     
    */

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true)); // Iniciar el escaneo

    /*
    se inicializa una variable "ap_num" que almacenará el numero de redes wifi escaneadas.
    además se crea un vector de 20 posiciones que guardará los datos de las redes escaneadas.
    */
    uint16_t ap_num = 20; // Máximo número de puntos de acceso a listar
    wifi_ap_record_t ap_records[20];

    /*
    El comando esp_wifi_scan_get_ap_records() devuelve el numero de redes y la informacion
    de cada una en un puntero hacia las variables que se le pasan como parametro.
    */

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records)); // Obtener resultados

    //Se imprimen las redes WiFi existentes:

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
    // Se inicializa la memoria flash
    ESP_ERROR_CHECK(nvs_flash_init());

    // Se inicializa la conexion wifi
    wifi_init();

    // Se configura el ESP en modo estación y se empieza a usar el ESP en modo estacion:
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // Configura en modo estación
    ESP_ERROR_CHECK(esp_wifi_start()); //empieza el modo estación

    while (true) {
        ESP_LOGI("WiFi Scan", "Iniciando escaneo de redes WiFi...");
        //inicia el escaneo de redes y las imprime en el monitor serial
        wifi_scan();
        vTaskDelay(pdMS_TO_TICKS(10000)); // Espera 10 segundos entre escaneos
    }
}
