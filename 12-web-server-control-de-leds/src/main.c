/*
Esta practica consiste en escanear los access point de la red wifi

https://youtube.com/playlist?list=PLmQ7GYcMY-2JV7afZ4hiekn8D6rRIgYfj&si=cQHAoQEpCTzz2vWw

video #11
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/*
la siguiente libreria se usa para inicializar el handler de eventos
wi-fi.

En ESP framework, el wi-fi trabaja con handlers como "connect handler",
"disconnect handler, etc..."
*/
#include "esp_event.h"
/*
añadir la libreria de wifi de la ESP32
*/
#include "esp_wifi.h"
/*
añadir la libreria de flash
*/
#include "nvs_flash.h"

//Funcion para escanear wifi
void scann(){
    /*
    se crea una estructura que establece los parametros de los access
    points en la red
    */
    wifi_scan_config_t scan_config = {
        .ssid = 0, //nombre de la red
        .bssid = 0, //dirección mac de la red
        .channel = 0, //a que canal esta conectado el broadcast
        /*
        el wifi trabaja normalmente a 2.4GHz. los canales vienen de la
        frecuencia de emision, dependiendo de si estan entre los 2.410GHz,
        2.420GHz, 2.430GHz, etc... la red estara ubicada entre el canal 1
        hasta el canal 14.

        en algunos paises los canales suelen ir unicamente entre el 1 y
        el 11
        */
        .show_hidden = true, //si es true se mostraran las redes ocultas
    };

    printf("empezando escaneo\n");

    /*
    a continuacion se inicia el escaneo con el metodo esp_wifi_scan_start()
    este tiene dos parametros:

    1) un puntero a la estructura de parametros de los access point
    2) bool block: si este parametro es true, esta API bloqueara al caller
       antes de que el escaneo se complete. de lo contrario retornará
       inmediatamente. 

    codigo:

    esp_wifi_scan_start(&scan_config,true);

    ahora si se busca en la documentacion, se puede observar que las funciones
    retornan esp ok si no hay ningun problema y luego continuan el proceso. sin 
    embargoo si hay un problema, podemos ver la razon del error en la 
    terminal. A esto se le puede añadir que se haga un get reset mediante
    el metodo ESP_ERROR_CHECK()
    */
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config,true));
    printf("completado\n");

    /*
    se crea una variable que almacenará el numero de access points en la
    red.
    */
    uint16_t ap_num;
    /*
    hay una estructura especial en espidf para obtener el escaneo de la red. 
    los access points del wifi graban y dan sus nombres como ap records y se
    pueden almacenar en un arreglo de 20 posiciones.

    20 es el maximo numero de redes que pueden ser almacenadas aunque este valor
    puede ser incrementado como se desee.
    */
    wifi_ap_record_t ap_records[20];
    /*
    ahora se escriben los valores escaneados en ese arreglo. para ello se 
    usa el metodo esp_wifi_scan_get_ap_records() el cual tiene dos parametros:

    1) un puntero hacia la variable donde se almacenará el numero de redes 
       escaneadas.
    2) el lugar donde serán almacenados los AP records.
    */
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));

    /*
    a continuacion se imprimen todos los parametros de la red con una cadena
    de caracteres dinamica:
    */
    printf("Se encontraron %d puntos de acceso: \n", ap_num);

    printf("        SSID        | CHANNEL | RSSI | MAC \n\n");

    for (int i = 0; i < ap_num; i++)
    {
        printf("%32s | %7d | %4d | %02x:%02x:%02x:%02x:%02x:%02x ",
               ap_records[i].ssid,
               ap_records[i].primary,
               ap_records[i].rssi,
               *ap_records[i].bssid,
               *ap_records[i].bssid+1,
               *ap_records[i].bssid+2,
               *ap_records[i].bssid+3,
               *ap_records[i].bssid+4,
               *ap_records[i].bssid+5
               );
    }
}

void app_main() 
{
    /*
    Se inicializa una flash no volatil y el tcpip
    */
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT(); 
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(3000));
        scann();
    }
    

}