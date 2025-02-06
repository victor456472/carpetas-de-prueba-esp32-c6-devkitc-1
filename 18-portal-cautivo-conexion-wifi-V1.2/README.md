<!--Usar el atajo Ctrl + Shift + P y escribir "Create Table of Contents" para crear tabla de contenido-->
# Tabla de Contenido
- [Tabla de Contenido](#tabla-de-contenido)
- [Información general](#información-general)
- [información tecnica](#información-tecnica)
  - [ubicacion de archivos SPIFFS dentro del proyecto de platformIO](#ubicacion-de-archivos-spiffs-dentro-del-proyecto-de-platformio)
  - [configuración de la memoria flash de la ESP32-C6-DevKitC-1](#configuración-de-la-memoria-flash-de-la-esp32-c6-devkitc-1)
  - [tabla de particiones](#tabla-de-particiones)
  - [Configuracion del archivo platformio.ini](#configuracion-del-archivo-platformioini)
  - [Subir archivos a la memoria flash del ESP32](#subir-archivos-a-la-memoria-flash-del-esp32)
  - [Acerca del programa cargado en la ESP32](#acerca-del-programa-cargado-en-la-esp32)
    - [1. Capa principal](#1-capa-principal)
    - [2. Capa 2](#2-capa-2)
      - [**2.1. inicializar sistema wifi**](#21-inicializar-sistema-wifi)
        - [**2.1.1 Estructura**:](#211-estructura)
        - [**2.1.2 Descripción**](#212-descripción)
          - [**2.1.2.1 inicializar sistema de redes en la ESP32**](#2121-inicializar-sistema-de-redes-en-la-esp32)
          - [**2.1.2.2 crear bucle de eventos de red principal de la ESP32**](#2122-crear-bucle-de-eventos-de-red-principal-de-la-esp32)
          - [**2.1.2.3 Inicializar el controlador wifi con la configuración por defecto (ESP-IDF)**](#2123-inicializar-el-controlador-wifi-con-la-configuración-por-defecto-esp-idf)
    - [3. Capa 3](#3-capa-3)
    - [4. Capa 4](#4-capa-4)
    - [1. inicializar SPIFFS](#1-inicializar-spiffs)
    - [2. Inicializar servidor web (modificado)](#2-inicializar-servidor-web-modificado)


# Información general

El presente codigo muestra como se puede inicializar spiffs para cargar un archivo html a la memoria flash de la ESP32-C6-DevKit-C1. Esta accion tiene ciertas ventajas respecto a utilizar codigo html directamente en el codigo del programa:

1) mantiene el codigo HTML separado del codigo C. esto mejora la legibilidad y la mantenibilidad del proyecto, especialmente si el archivo es grande o complejo

2) se puede modificar el contenido HTML simplemente editando el archivo HTML en la carpeta data y volviendo a cargar SPIFFS, sin necesidad de recompilar todo el codigo del firmware

3) permite almacenar no solo archivos HTML sino tambien otros archivos estaticos como CSS, Javascript o imágenes haciendo posible un servidor web mas robusto.

4) reduce el peso del cosigo C en memoria porque no se necesita incluir grandes cadenas de texto HTML dentro del codigo.

5) se pueden usar herramientas externas como editores web para diseñar el contenido HTML sin preocuparse por la sintaxis en C

a pesar de esto tambien tiene algunas desventajas asociadas:

1) Requiere configurar particiones SPIFFS en partitions.csv, compilar y subir los archivos estaticos. Esto puede ser una curva de aprendizaje compleja para principiantes.

2) si se quieren realizar cambios rapidos, se necesita editar los archivos, construir SPIFFS, subirlos y reiniciar el ESP32

3) Aunque SPIFFS es eficiente, ocupa espacio en la memoria flash del ESP32. para proyectos pequeños esto puede ser innecesario.

SPIFFS es mucho mas apropiado para proyectos medianos o grandes, los cuales deben manejar multiples recursos estaticos como HTML, CSS, JavaScript e imagenes. tambien es apropiado cuando se desea un diseño modular facil de modificar para mantenimiento.

# información tecnica

## ubicacion de archivos SPIFFS dentro del proyecto de platformIO

para ubicar archivos SPIFFS debe crear una carpeta llamada "data" en la raiz del proyecto. dentro de esta carpeta podra almacenar todos los archivos que se subirán a la memoria flash de la ESP32-C6-DevKitC-1.

## configuración de la memoria flash de la ESP32-C6-DevKitC-1

por defecto, el IDE de platformIO reconoce que la ESP32-C6-DevKit-C1 tiene un total de 2MB de memoria flash. Sin embargo, esta tiene realmente un total de 8MB que deben ser indicados a la plataforma antes de configurar el archivo **platformio.ini**; subir los SPIFFS al ESP; y subir el programa. para ello dirijase a la consola de PlatforIO y escriba lo siguiente:

```bash
pio run -t menuconfig
```

esto abrira el menú de configuracion de dispositivos IoT para la ESP32. en este menú debe ingresar con la tecla "enter" a:

```bash
Serial flasher config -> Flash size
```

donde podrá cambiar el tamaño de la memoria flash a 8MB. para seleccionar una de las opciones que se muestran en Flash size debe presionar la tecla "space" y para guardar los cambios debe presionar la tecla "s".

para salir del menú presione la tecla "esc" hasta que vuelva a aparecer la consola de comandos.

**Nota:** esta accion solo se realiza una sola vez y para este caso ya fue realizada por lo cual no es necesario que usted la repita si clonó este proyecto.

## tabla de particiones

La tabla de particiones es un archivo de configuración que define cómo se organiza y divide la memoria flash del ESP32 en diferentes áreas, llamadas particiones. Cada partición tiene un propósito específico, como almacenar datos, el firmware de la aplicación, o sistemas de archivos como SPIFFS o FAT.

Esta tabla es esencial porque permite al ESP32 gestionar y acceder a diferentes partes de su memoria flash de manera eficiente y estructurada.

**¿Por Qué Es Necesaria la Tabla de Particiones?**

1. Organización de la Memoria Flash:
    - Divide la memoria flash en secciones con usos específicos.
    - Asegura que diferentes tipos de datos no se sobrescriban accidentalmente.

2. Flexibilidad: Permite personalizar cómo se usa la memoria flash según las necesidades del proyecto.

3. Compatibilidad con Bootloaders y OTA: Define particiones para almacenar múltiples versiones de firmware, facilitando actualizaciones Over-The-Air (OTA).

4. Soporte para Sistemas de Archivos: Reservar particiones para sistemas como SPIFFS o FAT es indispensable para usar memoria flash como almacenamiento de archivos.

**Formato de la Tabla de Particiones**

La tabla de particiones generalmente se define en un archivo CSV (partitions.csv) ubicado en la carpeta raiz del proyecto.

en este caso la tabla de particiones se ve se la siguiente forma:

| Name      | Type  | SubType | Offset   | Size     | Flags |
|-----------|-------|---------|----------|----------|-------|
| nvs       | data  | nvs     | 0x9000   | 0x5000   |       |
| otadata   | data  | ota     | 0xe000   | 0x2000   |       |
| app0      | app   | ota_0   | 0x10000  | 0x180000 |       |
| spiffs    | data  | spiffs  | 0x190000 | 0x70000  |       |

cada una de las particiones tiene un proposito aunque en este caso solo se usa la particion app0, nvs y spiffs.

en el caso de la particion **nvs** se usa como almacenamiento persistente de datos clave-valor que suele guardar lkas configuraciones de las credenciales de Wi-Fi (SSID y contraseña). este esta definido en un tamaño de 0x5000 que corresponde a 20KB.

la particion **otadata** se usa para almacenar informacion de control para las actualizaciones OTA (Over-The-Air). en este caso se define con un tamaño de 0x2000 u 8KB

la partición **app0** contiene el firmware principal de la aplicación y es uno de los slots OTA para almacenar el codigo binario del programa que se ejecuta en el ESP32-DevKit_C6. en este caso se ha definido con un tamaño de 0x180000 que corresponde a 1.5MB

Finalmente, la partición **SPIFFS** se usa como sistema de archivos ligero para almacenar archivos html, css, javascript, imagenes, etc... este se define en un tamaño de 0x7000 que corresponde a 448KB

**campos de la tabla de particiones**

1. Nombre:

    - Identifica la partición.
    - Se usa para referenciar esta partición en el código, por ejemplo, "nvs" o "spiffs".

2. Tipo:

    - Define el tipo de partición. Valores comunes:
        - app: Almacena el firmware de la aplicación.
        - data: Almacena datos. Puede incluir configuraciones, sistemas de archivos, datos OTA, etc.

3. Subtipo:

    - Especifica un subtipo dentro del tipo de partición.
    - Subtipos comunes para data:
        - nvs: Almacén de valores clave-valor no volátiles (NVS).
        - ota: Datos de actualización OTA.
        - spiffs: Sistema de archivos SPIFFS.
    - Subtipos comunes para app:
        - factory: Firmware de fábrica.
        - ota_0, ota_1: Firmwares para actualizaciones OTA.
4. Dirección Offset:

    - Especifica dónde comienza la partición en la memoria flash.
    - Se define en hexadecimal. Por ejemplo, 0x10000 indica que la partición comienza en la dirección 0x10000.

5. Tamaño:

    - Indica el tamaño de la partición en bytes.
    - También se define en hexadecimal. Por ejemplo, 0x100000 equivale a 1 MB.

6. Etiqueta (opcional):

    - Proporciona un nombre simbólico para la partición que puede usarse en el código para acceder a ella.

## Configuracion del archivo platformio.ini

es necesario hacer unas configuracuiones adicionales al archivo platformio.ini para que el IDE prepare el proyecto antes de usar SÏFFS.

la primera configuracion se llama **board_build.filesystem** la cual indica cual sera el sistema de archivos que se usará en el proyecto. en este caso es SPIFFS (SPI Flash Files System).

la segunda configuracion se llama **board_build.partitions** la cual especifica el archivo CSV que define el esquema de particiones para la memoria flash del ESP32. en este caso el archivo se llama partitions.csv

finalmente, la configuracion **board_build.fs_dir** permite indicar la carpeta local donde se encuentran los archivos que se desean cargar al sistema de archivos SPIFFS. es este caso es la carpeta **data**

El contenido del archivo **platformio.ini** es el siguiente:

```ini
[env:esp32-c6-devkitc-1]
platform = espressif32
board = esp32-c6-devkitc-1
framework = espidf
board_build.filesystem = spiffs
board_build.partitions = partitions.csv
board_build.fs_dir = data
```

## Subir archivos a la memoria flash del ESP32

esta acción es necesaria antes de cargar el programa a la ESP32-C6-DevKitC-1, de lo contrario el servidor no encontrara el formulario http que debe retornarle al cliente. para ello asegurese de que en su proyecto se hayan realizado los pasos anteriores a esta sección (Si usted clonó este proyecto no tiene que preocuparse). Luego abra la terminal de comandos de platformIO y escriba lo siguiente:

1. Limpiar cualquier compilacion previa:

```bash
pio run --target clean
```

2. Construir SPIFFS:

```bash
pio run --target buildfs
```

3. subir SPIFFS al ESP32-C6-DevKitC-1:

```bash
pio run --target uploadfs
```
Finalmente, podrá construir y subir el programa a la ESP32-C6-DevKitC-1

## Acerca del programa cargado en la ESP32

En construccion ...

### 1. Capa principal

En construccion ...

### 2. Capa 2

En construccion ...

#### **2.1. inicializar sistema wifi**

##### **2.1.1 Estructura**:

<img src="assets\img\wifi_system_init_estructura.png" alt="Diagrama_2_" width="600">

##### **2.1.2 Descripción**

la funcion [wifi_system_init()](./src/main.c) se encarga de asegurar que la memoria no volatil y los eventos del sistema estén correctamente configurados. es necesario llamarla al inicio de un programa que usa wifi.

###### **2.1.2.1 inicializar sistema de redes en la ESP32**

El sistema de redes en la ESP32 es la parte del firmware que maneja la conectividad a redes Wi-Fi, Ethernet y otras interfaces de comunicación de datos. ESP-IDF (el framework de desarrollo de Espressif) proporciona una arquitectura de red modular para gestionar estas conexiones.

Cuando se ejecuta esp_netif_init(), se inicializa el subsistema de red, que permite:

* Configurar la ESP32 como cliente (STA) o punto de acceso (AP).
* Administrar la asignación de direcciones IP mediante DHCP.
* Manejar eventos de red, como la conexión y desconexión de Wi-Fi.
* Usar protocolos de red como TCP, UDP y HTTP.

esp_netif significa ESP Network Interface y en resumen es una capa de abstracción sobre las interfaces de red que permite crearlas usando simples comandos. por ejemplo para el caso de la interfaz STA y AP se usa respectivamente:

* esp_netif_create_default_wifi_sta()
* esp_netif_create_default_wifi_ap()

Si no se inicializa el sistema de redes, estas interfaces no pueden ser creadas.

*Enlaces web relacionados:*
* [¿Qué es la capa de red? | Capa de red vs. capa de Internet](https://www.cloudflare.com/es-es/learning/network-layer/what-is-the-network-layer/)

* [ESP-NETIF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_netif.html#initialization)

###### **2.1.2.2 crear bucle de eventos de red principal de la ESP32**

El bucle de eventos de red es un mecanismo en ESP-IDF que permite manejar eventos asincrónicos relacionados con Wi-Fi, Ethernet y TCP/IP en la ESP32. Su propósito es notificar al programa cuando ocurre un evento específico, como:

* Conexión o desconexión Wi-Fi.
* Obtención o pérdida de dirección IP.
* Conexión o desconexión de dispositivos en un punto de acceso.

La función clave para activar este sistema es [esp_event_loop_create_default()](./src/main.c) la cual inicializa el bucle de eventos, permitiendo que el sistema reaccione automáticamente a cambios en la red.

El sistema de eventos en ESP32 funciona con tres componentes principales:

1. eventos (esp_event_base_t) los cuales representan categorias de eventos como por ejemplo:

    * WIFI_EVENT → Eventos relacionados con Wi-Fi.
    * IP_EVENT → Eventos relacionados con direcciones IP.
    * ETH_EVENT → Eventos de Ethernet.

2. Manejadores de eventos (esp_event_handler_t) los cuales son funciones que responden cuando ocurre un evento y se registran con esp_event_handler_instance_register() como por ejemplo:

    ```c
    static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGI("WIFI", "Wi-Fi desconectado, intentando reconectar...");
            esp_wifi_connect();
        }
    }
    ```
    en este caso el manejador intenta reconectarse automáticamente si el ESP32 pierde conexion Wi-Fi.

3. Despachador de eventos (esp_event_loop_run()) el cual llama a los manejadores registrados cada vez que ocurre un evento. esto evita que el código principal tenga que comprobar manualmente el estado de la red.

Los principales eventos de red en la ESP32 cuando se trabaja con Wi-Fi son:

| Evento                          | Descripción                            |
|---------------------------------|----------------------------------------|
| `WIFI_EVENT_STA_START`          | Wi-Fi en modo STA ha iniciado.        |
| `WIFI_EVENT_STA_CONNECTED`      | Se ha conectado a una red Wi-Fi.      |
| `WIFI_EVENT_STA_DISCONNECTED`   | Se ha perdido la conexión a la red.   |
| `WIFI_EVENT_AP_STACONNECTED`    | Un dispositivo se conectó al AP.      |
| `WIFI_EVENT_AP_STADISCONNECTED` | Un dispositivo se desconectó del AP.  |
| `IP_EVENT_STA_GOT_IP`           | Se ha obtenido una dirección IP.      |
| `IP_EVENT_STA_LOST_IP`          | Se ha perdido la dirección IP.        |

###### **2.1.2.3 Inicializar el controlador wifi con la configuración por defecto (ESP-IDF)**

la ESP32 cuenta con un controlador Wi-Fi integrado que necesita ser inicializado antes de usarlo. para ello se suele seguir el siguiente flujo de programa:

1. Inicializar el subsistema de red con esp_netif_init().
2. Configurar y crear las interfaces de red (STA/AP) con esp_netif_create_default_wifi_sta() o esp_netif_create_default_wifi_ap(). Este paso es opcional, se puede realizar luego de iniciar el controlador Wi-Fi
3. Crear el bucle de eventos con esp_event_loop_create_default() 
4. Inicializar el controlador Wi-Fi con esp_wifi_init().

El paso 4 es el que usa la estructura wifi_init_config_t y la macro WIFI_INIT_CONFIG_DEFAULT().

La estructura wifi_init_config_t define los parametros de configuración del Wi-Fi antes de iniciarlo. Es una estructura que agrupa configuraciones del driver Wi-Fi y con la macro WIFI_INIT_CONFIG_DEFAULT() se establece de esta forma:


| **Parámetro**                     | **Descripción** |
|------------------------------------|----------------|
| `.osi_funcs = &g_wifi_osi_funcs` | Funciones del sistema operativo utilizadas por el driver Wi-Fi. |
| `.wpa_crypto_funcs = g_wifi_default_wpa_crypto_funcs` | Funciones criptográficas WPA utilizadas en conexiones STA. |
| `.static_rx_buf_num = CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM` | Número de buffers RX estáticos para recibir datos Wi-Fi. |
| `.dynamic_rx_buf_num = CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM` | Número de buffers RX dinámicos asignados bajo demanda. |
| `.tx_buf_type = CONFIG_ESP_WIFI_TX_BUFFER_TYPE` | Tipo de buffer TX usado para transmisión de paquetes. |
| `.static_tx_buf_num = WIFI_STATIC_TX_BUFFER_NUM` | Número de buffers TX estáticos asignados. |
| `.dynamic_tx_buf_num = WIFI_DYNAMIC_TX_BUFFER_NUM` | Número de buffers TX dinámicos asignados bajo demanda. |
| `.rx_mgmt_buf_type = CONFIG_ESP_WIFI_DYNAMIC_RX_MGMT_BUF` | Tipo de buffer para la recepción de paquetes de gestión. |
| `.rx_mgmt_buf_num = WIFI_RX_MGMT_BUF_NUM_DEF` | Número de buffers dedicados a la recepción de paquetes de gestión. |
| `.cache_tx_buf_num = WIFI_CACHE_TX_BUFFER_NUM` | Número de buffers TX en caché. |
| `.csi_enable = WIFI_CSI_ENABLED` | Habilita la recopilación de información del estado del canal (**CSI**). |
| `.ampdu_rx_enable = WIFI_AMPDU_RX_ENABLED` | Habilita la recepción de tramas agregadas (**AMPDU RX**). |
| `.ampdu_tx_enable = WIFI_AMPDU_TX_ENABLED` | Habilita la transmisión de tramas agregadas (**AMPDU TX**). |
| `.amsdu_tx_enable = WIFI_AMSDU_TX_ENABLED` | Habilita la transmisión de tramas agregadas **AMSDU** (más eficiente en redes congestionadas). |
| `.nvs_enable = WIFI_NVS_ENABLED` | Habilita el almacenamiento de configuración Wi-Fi en **NVS**. |
| `.nano_enable = WIFI_NANO_FORMAT_ENABLED` | Habilita el modo **Nano** para reducir el uso de memoria en `printf()` y `scanf()`. |
| `.rx_ba_win = WIFI_DEFAULT_RX_BA_WIN` | Tamaño de la ventana **Block Ack (BA)** en recepción. |
| `.wifi_task_core_id = WIFI_TASK_CORE_ID` | Núcleo de la **ESP32** donde se ejecutará la tarea Wi-Fi. |
| `.beacon_max_len = WIFI_SOFTAP_BEACON_MAX_LEN` | Tamaño máximo de los **beacons** en modo AP. |
| `.mgmt_sbuf_num = WIFI_MGMT_SBUF_NUM` | Número de buffers cortos para paquetes de gestión (**mínimo 6, máximo 32**). |
| `.feature_caps = WIFI_FEATURE_CAPS` | Habilita funciones avanzadas del controlador Wi-Fi. |
| `.sta_disconnected_pm = WIFI_STA_DISCONNECTED_PM_ENABLED` | Habilita el **modo de ahorro de energía en STA** cuando está desconectado. |
| `.espnow_max_encrypt_num = CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM` | Número máximo de dispositivos soportados con cifrado en **ESP-NOW**. |
| `.tx_hetb_queue_num = WIFI_TX_HETB_QUEUE_NUM` | Número de colas **HE TB** para transmisión en **modo Wi-Fi 6 (HE TB PPDU)**. |
| `.dump_hesigb_enable = WIFI_DUMP_HESIGB_ENABLED` | Habilita el volcado del campo **HE SIG-B**, usado en Wi-Fi 6. |
| `.magic = WIFI_INIT_CONFIG_MAGIC` | Número mágico usado para verificar la correcta inicialización del Wi-Fi. |


### 3. Capa 3

### 4. Capa 4

### 1. inicializar SPIFFS

Estructura de la función:

<img src="assets\img\Imagen2.png" alt="Diagrama_2_" width="600">

El sistema de archivos SPIFFS se registra con el Virtual File System (VFS) del ESP32 usando una estructura. Esto permite que el acceso a SPIFFS se realice a través de las funciones estándar de C, como fopen, fwrite, fread, etc.

**descripcion de los campos de la estructura**

- base_path (tipo: const char*): 
    - Especifica el punto de montaje en el sistema de archivos.
    - Este es el prefijo que usarás para acceder a los archivos almacenados en SPIFFS. Por ejemplo, si base_path es /spiffs, un archivo llamado config.txt será accesible como /spiffs/config.txt.

- partition_label (tipo: const char*): 
    - Define la etiqueta de la partición SPIFFS en la tabla de particiones.
    - Si se deja como NULL, se utiliza la partición predeterminada configurada en la tabla de particiones del ESP32.
    - Esto es útil si tienes múltiples particiones SPIFFS y deseas especificar cuál usar.

- max_files (tipo: size_t): 
    - Número máximo de archivos que se pueden abrir simultáneamente en SPIFFS.
    - Por ejemplo, si estableces max_files a 5, solo se podrán abrir 5 archivos al mismo tiempo.

- format_if_mount_failed (tipo: bool): 
    - Indica si el sistema debe formatear la partición si falla al intentar montarla.
    - Si es true, SPIFFS intentará formatear la partición y luego volver a montarla automáticamente.
    - Si es false, se registrará un error sin intentar formatear.

al final, se intenta montar la partición SPIFFS:

1. Si el montaje falla y format_if_mount_failed es true, formatea la partición automáticamente.
2. Si todo es exitoso, permite al sistema acceder a los archivos en el prefijo /spiffs.

para mayor informacion puede revisar los comentarios en
la funcion init_spiffs() del archivo src/main.c

### 2. Inicializar servidor web (modificado)

Para entender la modificación realizada a este bloque del programa dirijase al diagrama que se encuentra en la documentación: [ejemplo15-inicializar-servidor-web](https://github.com/victor456472/carpetas-de-prueba-esp32-c6-devkitc-1/tree/master/15-modoAP-servidor-web#inicializar-servidor-web-estructura). Podrá observar que en el diagrama representativo de la función de inicializacion del servidor web hay un bloque de **registro de rutas**. cada ruta se registra a través de una estructura la cual requiere de una **funcion manejadora**.

la modificacion se realiza precisamente en la **funcion manejadora de la ruta a la pagina principal** (/) ya que esta vez se enviará como respuesta al cliente el formulario html a través de un archivo .html en vez de usar una constante del tipo char dentro del codigo. para ello es necesario hacer lectura del SPIFF html almacenado en la memoria flash mediante el siguiente procedimiento:

<img src="assets\img\Imagen3.png" alt="Diagrama_3" width="900">

es importante aclarar que cada vez que el programa lee
un fragmento del archivo .html hay un indicador interno que informa sobre el grupo de datos fueron leidos lo cual permite que en la siguiente iteracion se lea unicamente el siguiente grupo de datos contenidos en el archivo. este indicador se maneja dentro de la funcion fread() de la libreria <stdio.h>. 

la lectura del archivo html se hace por fragmentos de informacion debido al limite de tamaño impuesto por el tipo de dato del búfer (arreglo char) el cual requiere de un tamaño fijo. en este caso se escogio 1024 bytes pero puede hacerse mas grande o mas pequeño dependiendo de la aplicación.

Tambien debe haber un conteo del numero de datos leidos en cada fragmento para poder determinar cuando va a finalizar la lectura del archivo. este conteo se hace a traves de una variable llamada "read_bytes".

Supongamos que el archivo tiene 2500 bytes y el tamaño del búfer es de 1024 bytes. En este caso, el flujo de lectura del archivo puede ejemplificarse de la siguiente forma:

| Iteración | Bytes disponibles | read_bytes | Acción                |
|-----------|-------------------|------------|-----------------------|
| 1         | 2500              | 1024       | Se envían 1024 bytes  |
| 2         | 1476              | 1024       | Se envían 1024 bytes  |
| 3         | 452               | 452        | Se envían 452 bytes   |
| 4         | 0                 | 0          | Bucle termina (EOF)   |

donde EOF significa "End Of File" o "fin del archivo"

para mayor informacion puede revisar los comentarios en la funcion root_handler() del archivo src/main.c

para aprender mas sobre SPIFFS, tabla de particiones y el uso de la memoria flash se recomienda visitar el siguiente enlace:

https://www.youtube.com/watch?v=V9-cgXag4Ko&t=242s




