/*
INTERRUPCIONES EXTERNAS

Nota: falta por probar en laboratorio

Asuma que un programa X esta ejecutandose, primero se ejecuta el
bloque A, luego el bloque B, luego el bloque C:

     _____________
    |             |
    |  Bloque A   |
    |_____________|                   
           |                             
           |
     interrupcion por boton --------------|
           |                              |    ______________________
           V                              |-->|                      |
     _____________                            |                      |
    |             |                           | CODIGO INTERRUPCION  |
    |   Bloque B  |                           |                      |
    |_____________|                       |-->|______________________|
           |                              |
     interrupcion por boton --------------|
           |
           V
     _____________
    |             |
    |  Bloque C   |
    |_____________|



Ahora, entre el codigo del bloque A y el bloque B se presiona un
boton. la idea basica de una interrupcion externa es que al presionar
el boton se active otro procceso determinado, interrumpiendo
temporalmente el programa X, ejecutando un proceso y luego regresando
al programa X desde el punto donde se activó la interrupción.

hay que tener en cuenta que la interrupcion no necesariamente debe 
esperar a que el bloque termine de ejecutar lo que esta haciendo para
activar el codigo de interrupcion. practicamente lo hace de inmediato
no importa que este iniciando, a mitad de proceso o finalizando proceso.
el CPU pondrá en pausa esa acción, se ejecutará el codigo de la
interrupción y luego se va a renaudar lo que se estaba trabajando.

para hacer esto se trabajará con un pushbutton como interrupcion
externa que alterna entre dos leds.
*/

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/semphr.h" 

 
#define ledB 10
#define ledY 11
#define button_interrupt 7

const char *tag = "main";

// se inicializa una variable contadora:
uint8_t contador = 0;

esp_err_t init_led(void);
esp_err_t init_interrupt(void); //funcion para inicializar interrupción

/*
IMPORTANTE: en las funciones de interrupción no es permitido usar
ninguna clase de print como ESP_LOG o printf.
*/
void funcion_interrupcion(void *args);

void app_main(void)
{
    init_led();
}
 
esp_err_t init_led()
{
    gpio_reset_pin(ledB);
    gpio_set_direction(ledB, GPIO_MODE_OUTPUT);
    gpio_reset_pin(ledY);
    gpio_set_direction(ledY, GPIO_MODE_OUTPUT);

    ESP_LOGI(tag, "inicialización de led completa");
    return ESP_OK;
}

esp_err_t init_interrupt(void)
{
    /*
    para poder configurar la interrupcion se debe utilizar el metodo
    gpio_config, el cual recibe como parametro un elemento tipo
    gpio_config_t el cual es una estructura compuesta por lo siguiente:

    1 - pin_bit_mask: la mascara de bits del pin
    2 - GPIO Mode: El modo del GPIO (entrada o salida)
    3 - GPIO Pullup: Si se va a habilitar un GPIO de PullUP
    4 - GPIO pulldown: Si se va a habilitar un GPIO de Pulldown
    5 - GPIO intr_type: El tipo de interrupcion
    */
    gpio_config_t pGPIOConfig;
    /*
    Parametro #1

    El parametro #1 es el grupo de pines donde se habilitaran las
    interrupciones. este parametro solo es recibido en mapa de bits,
    es decir, si el esp tuviese unicamente 4 pines el mapa de bits fuese:

    0b0000

    iniciando desde el pin 0 hasta el pin 3. para hacer la asignacion
    del mapa de bits usando numeros decimales se puede usar 1ULL que significa
    1 en formato unsigned long long de forma que se crea un mapa de 64
    bits de esta forma:

    0b00...x61 bits mas... 01

    indicando que se selecciona el pin 0. sin embargo, si se quiere
    seleccionar el pin 7 hay que desplazar el bit menos significativo
    7 veces a la derecha y para eso se escribe:

    1ULL << 7

    obteniendo asi:

    0b00...x54 bits mas...10000000

    indicando el pin al cual se le habilitará la interrupcion en mapa 
    de bits.
    */
    pGPIOConfig.pin_bit_mask = (1ULL << button_interrupt);
    pGPIOConfig.mode = GPIO_MODE_DEF_INPUT;
    pGPIOConfig.pull_up_en = GPIO_PULLUP_DISABLE;
    pGPIOConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;

    /*
    Parametro #5:
    - Establece el tipo de interrupción. Existen estos tipos:
      GPIO_INTR_DISABLE = Deshabilita la interrupción de GPIO
      GPIO_INTR_POSEDGE = flanco de subida
      GPIO_INTR_NEGEDGE = flanco de bajada
      GPIO_INTR_ANYEDGE = cualquier flanco (subida o bajada)
      GPIO_INTR_LOW_LEVEL = trigger de nivel bajo
      GPIO_INTR_HIGH_LEVEL = trigger de nivel alto
    */
    pGPIOConfig.intr_type = GPIO_INTR_NEGEDGE;

    gpio_config(&pGPIOConfig); // se configura la interrupción

    /*
    una vez se configura la interrupcion se debe habilitar utilizando
    el metodo gpio_install_isr_service() el cual recibe como parametro
    un solo dato:

    - intr_alloc_flags = si se le pasa un 1 se le permite hacer mas
      configuraaciones. en cambio si se le pasa un cero no se permite
      hacer mas configuraciones
    */

    gpio_install_isr_service(0);

    /*
    Finalmente, hay que agregar el pin a las interrupciones usando el
    metodo gpio_isr_handler_add() el cual tiene 3 parametros:

    1) El pin de interrupción
    2) La funcion que se ejecutará con la interrupción. Es importante
       que esta funcion reciba como parametro un void con puntero
       (void *)
    3) el ultimo parametro puede ser un NULL
    */
    gpio_isr_handler_add(button_interrupt, funcion_interrupcion, NULL);

    ESP_LOGI(tag, "inicialización de interrupción completada");

    return ESP_OK;
}

/*
cada vez que se ejecute la funcion interrupcion se va a alternar un
contador entre 1 y 0 cambiando los estados de dos leds.
*/
void funcion_interrupcion(void *args)
{
    contador ++;
    if (contador > 1)
    {
        contador = 0;
    }

    switch (contador)
    {
    case 0:
        gpio_set_level(ledB,1);
        gpio_set_level(ledY,0);
        break;
    case 1:
        gpio_set_level(ledB,0);
        gpio_set_level(ledY,1);
        break;
    default:
        break;
    }
}