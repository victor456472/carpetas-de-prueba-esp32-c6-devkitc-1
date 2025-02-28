# contenido

- [Información general](#información-general)
- [Información tecnica](#información-tecnica)
    - [Ubicacion de archivos spiffs dentro del proyecto de platformIO](#ubicacion-de-archivos-spiffs-dentro-del-proyecto-de-platformio)
    - [Configuración de la memoria flash de la esp32-c6-devkitc-1](#configuración-de-la-memoria-flash-de-la-esp32-c6-devkitc-1)
    - [Tabla de particiones](#tabla-de-particiones)
    - [Configuracion del archivo platformio.ini](#configuracion-del-archivo-platformioini)
    - [Subir archivos a la memoria flash del ESP32](#subir-archivos-a-la-memoria-flash-del-esp32)
    - [Acerca del programa cargado en la ESP32](#acerca-del-programa-cargado-en-la-esp32)
        - [1. inicializar spiffs](#1-inicializar-spiffs)
        - [2. inicializar servidor web (modificado)](#2-inicializar-servidor-web-modificado)

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

el  programa que se cargará en la ESP32 cumple la misma funcion que el [ejemplo15](https://github.com/victor456472/carpetas-de-prueba-esp32-c6-devkitc-1/tree/master/15-modoAP-servidor-web#informacion-general) sin embargo, esta vez se utilizará SPIFFS para poder almacenar el html.

la estructura general es la siguiente:

<img src="assets\img\Imagen1.png" alt="Diagrama_1" width="180">

en esta documentacion se va a expandir el bloque para iniciar SPIFFS y se expondrán algunas modificaciones realizadas dentro de la inicializacion del servidor web

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




