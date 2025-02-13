<!--Usar el atajo Ctrl + Shift + P y escribir "Create Table of Contents" para crear tabla de contenido-->
# **Tabla de Contenido**
- [**Tabla de Contenido**](#tabla-de-contenido)
- [**Información general**](#información-general)
- [**Información tecnica**](#información-tecnica)
  - [**Ubicacion de archivos SPIFFS dentro del proyecto de platformIO**](#ubicacion-de-archivos-spiffs-dentro-del-proyecto-de-platformio)
  - [**Configuración de la memoria flash de la ESP32-C6-DevKitC-1**](#configuración-de-la-memoria-flash-de-la-esp32-c6-devkitc-1)
  - [**Tabla de particiones**](#tabla-de-particiones)
  - [**Configuracion del archivo platformio.ini**](#configuracion-del-archivo-platformioini)
  - [**Subir archivos a la memoria flash del ESP32**](#subir-archivos-a-la-memoria-flash-del-esp32)
  - [**Acerca del programa cargado en la ESP32**](#acerca-del-programa-cargado-en-la-esp32)
    - [**1. Capa principal**](#1-capa-principal)
    - [**2. Capa 2**](#2-capa-2)
      - [**2.1. Inicializar sistema wifi**](#21-inicializar-sistema-wifi)
        - [**2.1.1 Algoritmo del programa**](#211-algoritmo-del-programa)
        - [**2.1.2 Descripción**](#212-descripción)
          - [**2.1.2.1 Inicializar sistema de redes en la ESP32**](#2121-inicializar-sistema-de-redes-en-la-esp32)
          - [**2.1.2.2 Crear bucle de eventos de red principal de la ESP32**](#2122-crear-bucle-de-eventos-de-red-principal-de-la-esp32)
          - [**2.1.2.3 Inicializar el controlador wifi con la configuración por defecto (ESP-IDF)**](#2123-inicializar-el-controlador-wifi-con-la-configuración-por-defecto-esp-idf)
      - [**2.2. Inicializar GPIOs**](#22-inicializar-gpios)
        - [**2.2.1 Algoritmo del programa**](#221-algoritmo-del-programa)
        - [**2.2.2 Descripción**](#222-descripción)
        - [**2.2.3 hardware asociado**](#223-hardware-asociado)
      - [**2.3. Configurar el ADC**](#23-configurar-el-adc)
        - [**2.3.1 Algoritmo del programa**](#231-algoritmo-del-programa)
        - [**2.3.2 Descripción**](#232-descripción)
          - [**2.3.1.1 Configurar e inicializar la unidad ADC**](#2311-configurar-e-inicializar-la-unidad-adc)
          - [**2.3.1.2 Configurar e inicializar canal del ADC**](#2312-configurar-e-inicializar-canal-del-adc)
        - [**2.3.3 hardware asociado**:](#233-hardware-asociado)
      - [**2.3. Establecer timer del ADC**](#23-establecer-timer-del-adc)
        - [**2.3.1 Algoritmo del programa**](#231-algoritmo-del-programa-1)
        - [**2.3.2 Definición**](#232-definición)
      - [**2.4. Inicializar led RGB**](#24-inicializar-led-rgb)
        - [**2.4.1 Algoritmo del programa**](#241-algoritmo-del-programa)
        - [**2.4.2 Descripción**](#242-descripción)
      - [**2.5. Inicializar sistema de archivos SPIFFS**](#25-inicializar-sistema-de-archivos-spiffs)
        - [**2.5.1 Algoritmo del programa**](#251-algoritmo-del-programa)
        - [**2.5.2 Descripción**](#252-descripción)
      - [**2.6. Establecer modo AP+STA**](#26-establecer-modo-apsta)
        - [**2.6.1 Algoritmo del programa**](#261-algoritmo-del-programa)
        - [**2.6.2 Descripción**](#262-descripción)
      - [**2.7. Inicializar servidor web**](#27-inicializar-servidor-web)
        - [**2.7.1 Algoritmo del programa**](#271-algoritmo-del-programa)
        - [**2.7.2 Descripción**](#272-descripción)
      - [**2.8. Iniciar eventos wi-fi**](#28-iniciar-eventos-wi-fi)
        - [**2.8.1 Algoritmo del programa**](#281-algoritmo-del-programa)
        - [**2.8.2 Descripción**](#282-descripción)
    - [**3. Capa 3**](#3-capa-3)
      - [**3.1. Lectura del sensor de CO2**](#31-lectura-del-sensor-de-co2)
        - [**3.1.1 Algoritmo del programa**](#311-algoritmo-del-programa)
        - [**3.1.2 Descripción**](#312-descripción)
    - [**4. Capa 4**](#4-capa-4)
    - [**2. Inicializar servidor web (modificado)**](#2-inicializar-servidor-web-modificado)


# **Información general**

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

# **Información tecnica**

## **Ubicacion de archivos SPIFFS dentro del proyecto de platformIO**

para ubicar archivos SPIFFS debe crear una carpeta llamada "data" en la raiz del proyecto. dentro de esta carpeta podra almacenar todos los archivos que se subirán a la memoria flash de la ESP32-C6-DevKitC-1.

## **Configuración de la memoria flash de la ESP32-C6-DevKitC-1**

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

## **Tabla de particiones**

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

## **Configuracion del archivo platformio.ini**

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

## **Subir archivos a la memoria flash del ESP32**

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

## **Acerca del programa cargado en la ESP32**

[ir a tabla de Contenido](#tabla-de-contenido)

Esta sección está en construcción ...

### **1. Capa principal**

[ir a tabla de Contenido](#tabla-de-contenido)

Esta sección está en construcción ...

### **2. Capa 2**

[ir a tabla de Contenido](#tabla-de-contenido)

Esta sección está en construcción ...

#### **2.1. Inicializar sistema wifi**

[ir a tabla de Contenido](#tabla-de-contenido)

##### **2.1.1 Algoritmo del programa**

<img src="assets\img\wifi_system_init_estructura.png" alt="Diagrama_2_" width="600">

##### **2.1.2 Descripción**

la funcion [wifi_system_init()](./src/main.c) se encarga de asegurar que la memoria no volatil y los eventos del sistema estén correctamente configurados. es necesario llamarla al inicio de un programa que usa wifi.

###### **2.1.2.1 Inicializar sistema de redes en la ESP32**

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

###### **2.1.2.2 Crear bucle de eventos de red principal de la ESP32**

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

#### **2.2. Inicializar GPIOs**
[ir a tabla de Contenido](#tabla-de-contenido)

##### **2.2.1 Algoritmo del programa**
<img src="assets\img\wifi_init_gpio_estructura.png" alt="wifi_init_gpio_estructura" width="800">

##### **2.2.2 Descripción**
Este es un metodo que permite inicializar los GPIOS del ESP32. hasta el momento solo se establece el GPIO 18 como entrada de pull down para poder conectar un botón que pueda restablecer el modo AP de la ESP32 en caso de algun inconveniente en el modo STA

##### **2.2.3 hardware asociado**

El GPIO18 recibe una señal binaria de entrada enviada por un pulsador de pull down externo al ESP32 (Importante armar el circuito que se muestra a continuación).

<img src="assets\img\circuito_pulsador_reset.png" alt="circuito_pulsador_reset" width="400">

#### **2.3. Configurar el ADC**
[ir a tabla de Contenido](#tabla-de-contenido)
##### **2.3.1 Algoritmo del programa**

<img src="assets\img\configurar_adc.png" alt="configurar_adc" width="700">

##### **2.3.2 Descripción**

la función `set_adc()` configura y habilita un ADC (Conversor Analógico-Digital) en la ESP32 utilizando el modo de lectura "oneshot", que permite realizar mediciones de voltaje bajo demanda a través del GPIO 5. 

###### **2.3.1.1 Configurar e inicializar la unidad ADC**

El proceso inicia con la configuracion de la unidad del ADC. para entender esto, es necesario saber que actualmente en la ESP32, el conversor Analógico-Digital (ADC) esta dividido en multiples unidades, cada una con sus propios canales. estas unidades permiten leer señales analogicas y convertirlas en valores digitales.

En ESP_IDF una "Unidad ADC" (`ADC_UNIT_X`) se refiere a un bloque de hardware dentro del chip ESP32 que realiza la conversion de señales analogicas a digitales. En la ESP32-C6, solo existe una unidad ADC (`ADC_UNIT_1`) sin embargo en ESP clasicas existen hasta dos unidades. 

Este listado muestra el numero de unidades que tienen diferentes modelos de ESP32:

| **Modelo**   | **Unidades ADC**           | **Canales Disponibles** |
|-------------|--------------------------|----------------------|
| ESP32       | `ADC_UNIT_1`, `ADC_UNIT_2` | 18 canales          |
| ESP32-S2    | `ADC_UNIT_1`               | 10 canales          |
| ESP32-S3    | `ADC_UNIT_1`, `ADC_UNIT_2` | 20 canales          |
| ESP32-C3    | `ADC_UNIT_1`               | 5 canales           |
| ESP32-C6    | `ADC_UNIT_1`               | 6 canales           |

*Nota: para mas detalles sobre unidades revisar [Resource Allocation](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/api-reference/peripherals/adc_oneshot.html#resource-allocation)*

Otro tema a tener en cuenta respecto a la configuracion de la unidad es el ULP que se refiere a un coprocesador de bajo consumo en algunas variantes del ESP32. Este permite realizar tareas de bajo consumo mientras la CPU principal está en modo de bajo consumo o suspensión.

Es posible habilitar o deshabilitar el modo ULP en el ADC para que este funcione mientras la CPU principal está dormida de forma que la lectura se haga en modo de bajo consumo. si se desactiva el ADC solo funciona cuando la CPU principal está activa, sin embargo se contará con la ventaja de tener mayor precisión y estabilidad en las mediciones además de poder usar el ADC con llamadas normales (`adc_oneshot_read()`)

El proceso de configuración de la unidad se realiza mediante la estructura `adc_oneshot_unit_init_cfg_t` y este debe ser inicializado mediante la función `adc_oneshot_new_unit`. para que la inicialización funcione es necesario ingresar la direccion de memoria del handler del ADC (`&adc_handle`) ya que el manejador del ADC se crea dinámicamente dentro de la función, y su direccion de memoria debe ser almacenada en `adc_handle`. Si no se pasa la direccion de memoria del handler, la función no podría modificar `adc_handle` y no se sabria donde está almacenado el ADC.

El flujo de este programa se puede ver a continuación:

<img src="assets\img\configurar_unidad_adc.png" alt="configurar_unidad_adc" width="1000">

###### **2.3.1.2 Configurar e inicializar canal del ADC**

para configurar el canal ADC se requiere de la estructura `adc_oneshot_chan_cfg_t` la cual permite especificar la resolución en bits del ADC así como la atenuación del voltaje de entrada.

para entender esto es necesario aclarar algunos conceptos de electronica digital:

cuando se dice que el ADC tiene una resolución de $n$ bits, quiere decir que, ante un determinado rango de voltaje de entrada, el ADC puede representar a su salida un valor numerico entre un limite inferior $out_{min}$ de 0 y un limite superior dado por la ecuación:

 $out_{max}=(2^{n_{bits}})-1$

 En este caso, fue configurada una resolución de 12 bits por lo cual puede representar un rango numerico entero entre 0 y 4095. sin embargo, tambien se pueden configurar las siguientes resoluciones:

| **Resolución** | **Rango numérico** |
|--------------|----------------|
| 9 bits      | 0-511          |
| 10 bits     | 0-1023         |
| 11 bits     | 0-2047         |
| 12 bits     | 0-4095         |
| 13 bits     | 0-8191         |

En lo que respecta al nivel de atenuación, el ADC de la ESP32-C6 funciona internamente con un voltaje de hasta 1.1 voltios, sin embargo este ADC tiene una resistencia interna que puede ser configurada para atenuar voltajes mas grandes que 1.1 voltios. 

los niveles de atenuacion que maneja ESP-IDF son:

* 2.5dB
* 6dB
* 12dB
* 11dB <-- Obsoleto

El objetivo principal es lograr que con el nivel de atenuacion escogido, se reduzca la amplitud pico de la señal de entrada a un nivel muy cercano a 1.1 voltios. para ello se sigue la relación:

$v_{in}=1.1 \times 10^{\frac{dB}{20}} [V]$

donde $v_{in}$ es el voltaje pico de la señal de entrada y $dB$ es el nivel de atenuación seleccionado.

El fabricante recomienda que se tomen voltajes de entrada menores a 2.5 voltios para que la lectura del ADC sea lineal. teniendo esto en mente, se seleccionó un nivel de atenuación de 6dB de forma que el voltaje pico de la señal de entrada se limite a un máximo de 2.2 voltios. Sin embargo, el sensor que se está utilizando trabaja con una señal analogica de hasta 5 voltios por lo cual fue necesario diseñar un divisor de voltaje para alcanzar el rango de tensión apropiado. Este diseño se explica en el apartado del [hardware asociado](#233-hardware-asociado) a esta función.

una vez establecida la estructura de configuración, se inicializa el canal del ADC mediante la función `adc_oneshot_config_channel` a la cual se le debe especificar handler del ADC, el canal ADC que se va a utilizar y la estructura de configuración del canal.

es importante aclarar que para el caso del ESP32-C6 se dispone de los canales 0 al 6 ubicados en los GPIOS con la misma numeración.

Con esto en mente, el algoritmo de configuracion e inicialización del ADC es el siguiente:

<img src="assets\img\configurar_canal_adc.png" alt="configurar_canal_adc" width="1000">

##### **2.3.3 hardware asociado**:

Para que el ADC pueda capturar la señal analogica que emite el sensor es necesario realizar una operacion de escalamiento mediante un circuito reductor. En este caso la señal de entrada tiene una amplitud pico de 5 voltios y el ADC requiere un voltaje maximo de 2.2 voltios. Esto implica que la ganancia del circuito reductor debe ser:

$$k=\frac{v_{out}}{v_{in}}=\frac{2.2[V]}{5[V]}=0.44$$

La estrategia utilizada para lograr esta ganancia es utilizar un divisor de voltaje con las resistencias $R3$ y $R4$. Se empleó el voltaje $v_{R3}$ como salida del reductor y se definio su resistencia de forma arbitraria en $10k\Omega$. para saber el valor correcto de $R4$ se usa la formula de ganancia del divisor de voltaje:

$$k=\frac{R3}{R3+R4}\rightarrow R4=R3\times \frac{(1-k)}{k}$$

Reemplazando los valores de $R3$ y $k$ se encuentra que se requiere una resistencia de $(140/11)k\Omega$ lo cual es aproximadamente igual a $12.7k\Omega$. Para darle estabilidad a la señal se agrega en paralelo un capacitor de $100\mu F$ teniendo en cuenta que el sensor no tiene variaciones de señal de alta frecuencia. Finalmente se asegura el acople de impedancias colocando un amplificador operacional en modo de seguidor de voltaje. El circuito completo de acondicionamiento de señal se muestra a continuación:

<img src="assets\img\Circuito_sensor_analogico.png" alt="Circuito_sensor_analogico" width="900">

#### **2.3. Establecer timer del ADC**

[ir a tabla de Contenido](#tabla-de-contenido)

##### **2.3.1 Algoritmo del programa**

<img src="assets\img\Establecer_temporizador_ADC.png" alt="Establecer_temporizador_ADC" width="800">

##### **2.3.2 Definición**

Se utiliza un timer de freeRTOS para poder realizar la lectura del ADC de forma periodica y asincronica. el procedimiento para configurar el timer se basa en la creacion de un manejador a través de la función (`xTimerCreate`) la cual permite establecer parametros como el periodo, el reset del timer y la función que se ejecutará una vez expirado el tiempo.

para este caso, el timer se establece con un periodo de 100ms con repetición. La función que se ejecuta con el timer se denomina `vTimerCallBack` y es la encargada de la [lectura del sensor de CO2](#31-lectura-del-sensor-de-co2)

es importante mencionar que los timers deben ser iniciados para que funcionen. En esta funcion simplemente se crea el handler con las configuraciones necesarias para poder iniciar el timer.

#### **2.4. Inicializar led RGB**

[ir a tabla de Contenido](#tabla-de-contenido)
  
##### **2.4.1 Algoritmo del programa**

<img src="assets\img\inicializar_led_RGB.png" alt="inicializar_led_RGB" width="800">

##### **2.4.2 Descripción**

La ESP32-C6-DevKitC-1 incorpora un LED RGB de referencia WS2812B. Esta gama de LEDS incorporan un chip de control integrado que permite acoplar varios leds a una sola linea de datos y controlarlos individualmente. Estos LEDS se caracterizan por tener una alta frecuencia de actualizacion, ademas de tener un alto brillo y eficiencia energetica.

La forma en la cual se controla este LED es a traves de un protocolo de comunacion serie el cual emplea 24 bits de datos: 8 para la asignacion del color verde, 8 para la asignacion del color rojo y 8 para la asignacion del color azul.

<img src="assets\img\protocolo_comunicacion_led.png" alt="protocolo_comunicacion_led" width="600">

cada byte puede establecer un nivel de intensidad entre 0 y 255.

***Nota:*** *revisar [datasheet del WS2812B](https://www.alldatasheet.com/datasheet-pdf/view/1179113/WORLDSEMI/WS2812B.html)*

Por fortuna, la libreria ubicada en [include/led_strip.h](include/led_strip.h) permite manejar facilmente la comunicacion con el LED a través del canal RMT. esta libreria establece una estructura de control con la función `led_init_strip()` que recibe como parametros:

1. canal RMT para controlar el color del LED
2. GPIO al cual está conectado el pin de datos del LED. Para el ESP32-C6-DevKitC-1 este es el GPIO8.
3. El numero de leds conectados al pin de datos. en este caso solo hay un LED.

cuando se crea la estructura, es posible acceder a los metodos que proporciona la libreria para controlar el led. Para esta función `init_led_strip()` simplemente se usa el metodo `clear()` que borra cualquier color que haya sido establecido en el LED con programas anteriores.

si se desea conocer mas sobre el hardware que rodea al LED en la ESP32 se recomienda visitar el [diagrama circuital de la ESP32-C6-DevKitC-1](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/_static/esp32-c6-devkitc-1/schematics/esp32-c6-devkitc-1-schematics_v1.4.pdf)

#### **2.5. Inicializar sistema de archivos SPIFFS**

[ir a tabla de Contenido](#tabla-de-contenido)

##### **2.5.1 Algoritmo del programa**

<img src="assets\img\Iniciar_spiffs.png" alt="Iniciar_spiffs" width="800">


##### **2.5.2 Descripción**

SPIFFS (SPI Flash File System) es un sistema de archivos ligero y diseñado para memorias Flash SPI NOR, como las que usan las placas ESP32 y ESP8266.
Permite almacenar, leer, escribir y gestionar archivos en la memoria Flash de la ESP32, funcionando como una unidad de almacenamiento. En este caso, el sistema se usara para almacenar archivos html, css, js, imagenes, entre otros recursos necesarios para el funcionamiento del portal cautivo.

El sistema de archivos SPIFFS se registra con el Virtual File System (VFS) del ESP32 usando una estructura que contiene los siguientes parametros:

| **Campo**                  | **Descripción** |
|----------------------------|----------------|
| `.base_path`               | Define el punto de montaje del sistema de archivos. Todos los archivos almacenados en SPIFFS estarán accesibles desde esta ruta. Ejemplo: `"/spiffs"`. |
| `.partition_label`         | Especifica el nombre de la partición SPIFFS en la tabla de particiones. Si se establece en `NULL`, se usará la partición predeterminada para SPIFFS. |
| `.max_files`               | Número máximo de archivos que pueden abrirse simultáneamente. Un valor común es `5`, dependiendo de la memoria disponible. |
| `.format_if_mount_failed`  | Si es `true`, la ESP32 intentará **formatear la partición** en caso de que falle el montaje del sistema de archivos. Esto ayuda a recuperar SPIFFS en caso de corrupción. |

Esto permite que el acceso a SPIFFS se realice a través de las funciones estándar de C, como fopen, fwrite, fread, etc.

El programa intenta montar la partición SPIFFS:

* Si el montaje falla y `format_if_mount_failed` es `true`, formatea la partición automáticamente.
* Si todo es exitoso, permite al sistema acceder a los archivos en el prefijo `/spiffs`.

para mayor informacion puede revisar los comentarios en
la funcion `init_spiffs()` del archivo `src/main.c`.

#### **2.6. Establecer modo AP+STA**

[ir a tabla de Contenido](#tabla-de-contenido)

##### **2.6.1 Algoritmo del programa**

<img src="assets\img\establecer_AP_STA.png" alt="establecer_AP_STA" width="1000">

##### **2.6.2 Descripción**

La función `wifi_set_AP_STA()` configura la ESP32 en modo Wi-Fi AP+STA, lo que permite que el dispositivo actúe simultáneamente como un punto de acceso y como un cliente de una red Wi-Fi existente. Esto es útil en aplicaciones donde la ESP32 debe estar conectada a una red mientras permite la conexión de otros dispositivos para configuración o comunicación local.

La función inicia con un mensaje informativo indicando que el proceso de configuración del Wi-Fi en modo AP+STA ha comenzado. Luego, se obtiene el modo Wi-Fi actual utilizando `esp_wifi_get_mode(&current_mode)`. Si esta llamada es exitosa, el valor de `current_mode` permitirá conocer en qué estado se encuentra la ESP32 antes de hacer cambios en la configuración. Si el dispositivo está en modo `WIFI_MODE_STA`, se detiene el Wi-Fi con `esp_wifi_stop()` antes de proceder con la nueva configuración. Se añade un retraso de 500 ms mediante `vTaskDelay(pdMS_TO_TICKS(500))` para evitar problemas durante la reconfiguración.

En caso de que `esp_wifi_get_mode()` no devuelva `ESP_OK`, se imprime una advertencia, pero el código continúa con la configuración. Esto permite que el sistema continúe funcionando incluso si no se pudo obtener el estado del Wi-Fi.

El siguiente paso es cambiar el modo Wi-Fi a `WIFI_MODE_APSTA` utilizando `esp_wifi_set_mode(WIFI_MODE_APSTA)`, lo que habilita la coexistencia del modo punto de acceso y estación en la ESP32. Antes de continuar, se verifica si ya existen interfaces de red (`ap_netif` y `sta_netif`), y en caso afirmativo, se eliminan con `esp_netif_destroy()`. Esto es necesario para evitar conflictos y asegurar que las interfaces se creen correctamente antes de iniciar el Wi-Fi.

Las nuevas interfaces de red se crean con `esp_netif_create_default_wifi_ap()` y `esp_netif_create_default_wifi_sta()`, que configuran las interfaces de red por defecto para el punto de acceso y el modo estación, respectivamente. Una vez creadas las interfaces, se configura la red AP mediante la estructura `wifi_config_t`. En esta estructura se define el SSID (`AP_SSID`), la contraseña (`AP_PASSWORD`), la longitud del SSID, el canal Wi-Fi en el que operará el punto de acceso, el número máximo de dispositivos conectados simultáneamente (`MAX_STA_CONN`) y el modo de autenticación (`WIFI_AUTH_WPA_WPA2_PSK`). Si la contraseña está vacía, se cambia el modo de autenticación a `WIFI_AUTH_OPEN`, lo que permite la creación de una red sin contraseña. a continuación se resumen los parametros de configuración del AP:

| **Parámetro**      | **Descripción** |
|--------------------|----------------|
| `AP_SSID`         | Nombre de la red Wi-Fi creada por la ESP32. |
| `AP_PASSWORD`     | Contraseña del punto de acceso. Si está vacía, el AP será abierto. |
| `ssid_len`        | Longitud del SSID (calculado automáticamente con `strlen(AP_SSID)`). |
| `channel`         | Canal Wi-Fi en el que operará el AP (por defecto 1). |
| `max_connection`  | Número máximo de dispositivos conectados simultáneamente. |
| `authmode`        | Modo de autenticación (`WIFI_AUTH_WPA_WPA2_PSK` o `WIFI_AUTH_OPEN` si no hay contraseña). |


La configuración del punto de acceso se aplica con `esp_wifi_set_config(WIFI_IF_AP, &wifi_config)`, asegurando que el punto de acceso se inicie con los parámetros definidos. Luego, el Wi-Fi se inicia con `esp_wifi_start()`, permitiendo que la ESP32 funcione en modo AP+STA. Se imprime un mensaje indicando que la interfaz de configuración está activa y se vuelve a obtener el modo Wi-Fi para confirmar el estado de la conexión.

Finalmente, la función enciende el LED RGB en color cian mediante `set_led_color(COLOR_CYAN)`, lo que indica visualmente que la ESP32 se encuentra en modo AP+STA. Esto permite que el usuario pueda reconocer el estado del dispositivo de manera inmediata.

El flujo de la función sigue una estructura que garantiza una transición estable entre los diferentes modos Wi-Fi. Se toman precauciones para evitar conflictos de configuración, asegurando que las interfaces de red sean destruidas y creadas de manera controlada. Además, la configuración del AP es flexible, permitiendo que el SSID y la contraseña sean personalizados o incluso dejando la red abierta si se requiere.

El uso de `ESP_LOGI()` y `ESP_LOGW()` facilita la depuración del código, proporcionando información en tiempo real sobre el estado de la conexión Wi-Fi. En caso de que se produzcan errores durante la configuración, estos se registran en la consola, lo que permite identificar y solucionar problemas de manera más sencilla.

La función `wifi_set_AP_STA()` es ideal para escenarios en los que se necesita que la ESP32 se conecte a una red Wi-Fi mientras proporciona una interfaz de acceso local. Esto es útil en aplicaciones de IoT, configuraciones iniciales de dispositivos embebidos y redes locales donde los usuarios pueden conectarse directamente a la ESP32 para control o configuración. La correcta gestión de las interfaces y la seguridad del punto de acceso garantizan un funcionamiento estable y eficiente del sistema.

#### **2.7. Inicializar servidor web**

[ir a tabla de Contenido](#tabla-de-contenido)

##### **2.7.1 Algoritmo del programa**

Esta sección está en construcción ...

##### **2.7.2 Descripción**

La función `start_http_server()` inicializa un servidor HTTP en la ESP32 utilizando la API `esp_http_server` de ESP-IDF. Este servidor permite a los clientes interactuar con la ESP32 a través de peticiones HTTP, facilitando la comunicación con dispositivos IoT y aplicaciones web.

El servidor maneja múltiples rutas, sirviendo contenido HTML, archivos JavaScript y permitiendo la interacción con la red Wi-Fi de la ESP32.

**Flujo de ejecución**

1. **Se inicializa la configuración del servidor HTTP** usando `HTTPD_DEFAULT_CONFIG()`.
2. **Se crea un manejador de servidor (`httpd_handle_t`) para registrar rutas.**
3. **Se inicia el servidor HTTP con `httpd_start()`**.
4. **Se registran los manejadores de URI (`httpd_register_uri_handler()`)**:
   - `/` → Página principal.
   - `/submit` → Recibe datos de formularios.
   - `/script.js` → Carga un archivo JavaScript desde SPIFFS.
   - `/scan` → Escanea redes Wi-Fi disponibles.
   - `/*` → Redirige cualquier ruta desconocida.

5. **Si el servidor no se inicia correctamente, se imprime un error.**

Una vez iniciado el servidor HTTP en la ESP32, se puede probar las rutas usando curl o un navegador.

**para verificar el servidor:**
```bash
curl http://192.168.4.1/
```
Esto devuelve el contenido de la página principal.

**Para enviar datos al servidor**
```bash
curl "http://192.168.4.1/submit?SSID=MiRed&PASSWORD=12345678"
```
Esto envía datos de configuración Wi-Fi.

**Para escanear redes Wi-Fi**
```bash
curl http://192.168.4.1/scan
```
Esto devuelve un JSON con la lista de redes Wi-Fi detectadas.

**Referencias**

* [ESP-IDF HTTP Server API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_server.html#:~:text=The%20HTTP%20Server%20component%20provides%20an%20ability%20for,to%20use%20the%20API%20exposed%20by%20HTTP%20Server%3A)
* [Ejemplo de servidor web en ESP-IDF](https://github.com/espressif/esp-idf/tree/master/examples/protocols/http_server)

#### **2.8. Iniciar eventos wi-fi**

[ir a tabla de Contenido](#tabla-de-contenido)

##### **2.8.1 Algoritmo del programa**

Esta sección está en construcción...

##### **2.8.2 Descripción**

La función `start_WIFI_events()` registra un manejador de eventos Wi-Fi en la ESP32 utilizando el sistema de eventos de ESP-IDF. Esto permite que la ESP32 **reaccione automáticamente** a eventos de conexión y desconexión, sin necesidad de consultar manualmente el estado de la red.

Esta funcionalidad es útil en aplicaciones IoT donde es necesario que la ESP32 se reconecte automáticamente si pierde la conexión Wi-Fi o maneje dispositivos que se conectan a su punto de acceso.

---

**Flujo de Ejecución**
1. **Registra un manejador de eventos Wi-Fi** con `esp_event_handler_instance_register()`.
2. **Especifica que capturará cualquier evento Wi-Fi (`ESP_EVENT_ANY_ID`)**.
3. **Asigna `event_handler()` como función manejadora** para procesar eventos.
4. **Guarda la instancia del manejador en `instance_any_id`** para poder desregistrarlo si es necesario.

---
**Código Explicado**
```c
void start_WIFI_events(void) {
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,                // Captura eventos Wi-Fi
        ESP_EVENT_ANY_ID,          // Captura cualquier ID de evento Wi-Fi
        &event_handler,            // Función encargada de manejar los eventos
        NULL,                      // No se pasa contexto adicional
        &instance_any_id           // Guarda la instancia del manejador
    ));
}
```
* `WIFI_EVENT`: Indica que estamos registrando eventos relacionados con la red Wi-Fi.
* `ESP_EVENT_ANY_ID`: Permite capturar cualquier evento Wi-Fi, sin necesidad de registrarlos individualmente.
* `&event_handler`: Apunta a la función que manejará los eventos Wi-Fi.
* `NULL`: No se pasa información adicional a event_handler().
* `&instance_any_id`: Guarda la instancia del manejador para poder desregistrarlo más tarde si es necesario.

---
**Eventos wifi manejados**

La función `start_WIFI_events()` permite capturar múltiples eventos de Wi-Fi en la ESP32, los cuales son procesados por event_handler().

| **Evento**                     | **Descripción** |
|---------------------------------|----------------|
| `WIFI_EVENT_STA_START`          | Se inicia la conexión Wi-Fi en modo STA. |
| `WIFI_EVENT_STA_CONNECTED`      | La ESP32 se conectó a una red Wi-Fi. |
| `WIFI_EVENT_STA_DISCONNECTED`   | Se perdió la conexión Wi-Fi. |
| `WIFI_EVENT_AP_START`           | El punto de acceso (AP) se inició. |
| `WIFI_EVENT_AP_STOP`            | El punto de acceso (AP) se detuvo. |
| `WIFI_EVENT_AP_STACONNECTED`    | Un dispositivo se conectó al AP de la ESP32. |
| `WIFI_EVENT_AP_STADISCONNECTED` | Un dispositivo se desconectó del AP de la ESP32. |

Estos eventos son capturados y procesados por `event_handler()`, permitiendo que la ESP32 reaccione en tiempo real a cambios en la red.

---
**Ejemplo de uso**

Para que la ESP32 maneje eventos Wi-Fi, es necesario registrar la función en la configuración del Wi-Fi.
Un ejemplo típico de uso en `app_main()` sería:

```c
void app_main(void) {
    // Configurar Wi-Fi con valores por defecto
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Registrar eventos Wi-Fi
    start_WIFI_events();

    // Configurar en modo estación (STA)
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
}
```
Esto permite que la ESP32:
* Detecte automáticamente eventos de conexión y desconexión Wi-Fi.
* Reaccione cuando se pierda la conexión (`WIFI_EVENT_STA_DISCONNECTED`).
* Maneje dispositivos que se conectan a su punto de acceso (`WIFI_EVENT_AP_STACONNECTED`).

---
**Ejemplo de Manejador de Eventos (event_handler())**

```c
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI("WiFi", "Conexión Wi-Fi perdida, intentando reconectar...");
        esp_wifi_connect();  // Intentar reconectar automáticamente
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI("WiFi", "Conectado a la red Wi-Fi.");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI("WiFi", "Un dispositivo se ha conectado al AP.");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        ESP_LOGI("WiFi", "Un dispositivo se ha desconectado del AP.");
    }
}
```

---
**Referencias**

* [ESP-IDF Event Loop Library](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/api-reference/system/esp_event.html)

### **3. Capa 3**

[ir a tabla de Contenido](#tabla-de-contenido)

#### **3.1. Lectura del sensor de CO2**

[volver a seccion 2.3.2](#232-definición)

##### **3.1.1 Algoritmo del programa**

<img src="assets\img\lectura_sensor_CO2.png" alt="lectura_sensor_CO2" width="1000">

##### **3.1.2 Descripción**

 este programa se ejecuta automáticamente cada vez que el temporizador expira, es decir, cada SAMPLE_PERIOD_MS=100 (milisegundos).
 
 Su función principal es leer el valor del ADC y registrarlo en el monitor serie.

 el parametro pxTimer es un manejador del temporizador freeRTOS. En esta ocasión no es usado pero podria servir para obtener información del temporizador si hubiera varios activos.


### **4. Capa 4**

[ir a tabla de Contenido](#tabla-de-contenido)

### **2. Inicializar servidor web (modificado)**

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




