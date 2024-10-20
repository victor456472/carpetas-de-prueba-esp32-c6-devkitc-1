# Repositorio de pruebas con ESP32-C6-DevKitC-1
    
    Se ha creado el repositorio con el fin de documentar las pruebas
    de desarrollo realizadas con la placa ESP32-C6-devkitc-1.

    En este primer commit se suben las siguientes carpetas numeradas:

    1-Blink-gpios-delays
    2-Libreria-ESP-LOG
    3-timer-con-freertos
    4-PWM-varios-canales

    cada carpeta corresponde a un proyecto en el cual se pone en practica
    un determinado aspecto de la programacion con el SDK de ESP-IDF
    a traves de platformio

    carpeta #1: temas tocados
    |
    |--> como configurar un puerto de entradas y salidas GPIO
    |
    |--> como realizar delays
    |
    |--> como hacer el blink de un GPIO y arrojar mensajes en monitor

    carpeta #2:
    |
    |--> contadores
    |
    |--> condicionales
    |
    |--> uso de la libreria ESP-LOG para la creacion de mensajes de
         información, advertencia y error

    carpeta #3:
    |
    |--> creacion de timers con eventos usando freertos

    carpeta #4:
    |
    |--> configuracion de puertos PWM para encender dos leds de forma
         gradual.

    Es importante mencionar que los codigos lo va a encontrar en:

    /carpeta-proyecto/src/main.c
