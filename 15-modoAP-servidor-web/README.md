## Informacion general
el presente codigo permite iniciar el ESP32-C6-DevKitC-1 en modo AP (Access Point) con el fin de alojar un servidor que emita un formulario web sobre el cual se ingrese un dato y este llegue a la ESP32.

al conectar la placa y subir el codigo, esta emitira una señal wifi con los siguientes parametros:

1. SSID: RED ESP32 VICTOR
2. PASSWORD: 12345678

cualquier dispositivo, ya sea un smartphone, tablet, PC o portatil puede conectarse a esta red.

una vez conectado puede acceder al sitio web del servidor escribiendo en el navegador la siguiente URL: http://192.168.4.1/. El sitio web es un formulario sobre el cual se puede ingresar un dato y enviar a la ESP32. una vez esta recibe el dato, lo imprime en el monitor serial.

## Informacion tecnica 

el codigo principal esta alojado en la funcion app_main() y tiene la siguiente estructura:

<img src="assets/imagen_principal.png" alt="imagen principal" width="150">

los dos primeros bloques del programa ya han sido estructurados en ejemplos anteriores por lo cual se expandirá unicamente el bloque para inicializar el servidor web

### inicializar servidor web (estructura)

<img src="assets\diagrama_inicializar_servidor.png" alt="diagrama: bloque de inicializacion de servidor" width="150">






