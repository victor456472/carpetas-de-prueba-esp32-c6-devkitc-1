#include <stdio.h>
#include "driver/gpio.h"            // Libreria para manejar GPIOs
#include "freertos/FreeRTOS.h"      //Libreria principal
#include "freertos/task.h"          //libreria para hacer delays

#define led1 8                      // definir el GPIO8 como salida 

uint8_t led_level = 0;              // esta variable se usa para enviar 1 o 0

esp_err_t init_led(void);           //se declara una funcion para configurar el led
esp_err_t blink_led(void);          //se declara una funcion para encender y apagar 
                                    //el led

/*
Nota: esp_err_t funciona como un try except: intenta ejecutar una función y si algo
pasa, te devuelve exactamente cual es el error. Esto es una buena practica al 
momento de programar
*/

void app_main(void)
{

/*

A continuación se crea una estructura cono la de arduino:

primero se invocan las funciones que solo se van a ejecutar una sola vez, esto
funciona como un void setup();
*/
    init_led();     //configurar led
/*
Luego se crea un ciclo while infinito para ingresar codigo que se repite cada
cierto tiempo. de esta forma se crea un void loop();
*/

    while(1){

        vTaskDelay(500 / portTICK_PERIOD_MS);
        /*
        la función vTaskDelay sirve para hacer un delay(). hay que tener en cuenta
        que  esta tiene como parametro un numero adimensional que indica el numero
        ticks en los que se quiere hacer una pausa. claramente la duracion de un
        tick depende de la frecuencia del controlador por lo cual se usa un
        atrubuto llamado "portTICK_PERIOD_MS para poder ingresar la entrada en
        ms.

        1ms = k [ms/tick] * 1tick
                |
                |
                V
        1tick = 1ms / (k [ms/tick])

        donde k es portTICK_PERIOD_MS.
        */

        blink_led();                       //alternar encendido y apagado del led
        
        //Luego se imprime en pantalla el estado del led:
        
        printf("estado del led: %u\n", led_level);

    }
}

//Funcion de configuracion de puerto para el LED:
esp_err_t init_led(void)
{
    /*

    primero se debe resetear el modo que tenia el GPIO8 antes de ser
    programado usando la función gpio_reset_pin:

    */

    gpio_reset_pin(led1);

    /*
    Luego se establecce el modo de operacion del GPIO8 como salida digital.

    para ello se utiliza la función gpio_set_direction() la cual tiene dos parametros:

    1- el numero del GPIO
    2- la direccion del GPIO que es lo mismo que indicar si es de entrada o salida.
       hasta ahora unicamente se sabe que para establecerlo como salida hay que
       escribir: GPIO_MODE_OUTPUT y se infiere que se debe usar GPIO_MODE_INPUT
       para establecerlo como entrada.
    
    Observe que esto sirve como un pinMode()
    */

    gpio_set_direction(led1, GPIO_MODE_OUTPUT);

    //si todo se ejecura se retorna que todo esta bien usando ESP_OK
    return ESP_OK;
}

//Alternar encendido y apagado del led:
esp_err_t blink_led(void)
{

    /*
    Al ser invocada la funcion blink_led se alterna el estado de led_level
    entre 1 y 0.  
    */

    led_level = !led_level;

    /*
    Ese estado se establece en el GPIO usando gpio_set_level(). Esto funciona
    como un digitalWrite()
    */

    gpio_set_level(led1,led_level);
    return ESP_OK;

}
