/*
COLAS

son el envio ordenado de datos de una tarea hacia otra, de modo que podamos
comunicarlas.
     ___________         _______________         ___________
    |           |       |               |       |           |
    |  tarea 1  | ----->|COLA:[5,3,8,9] |------>|  tarea 2  |
    |___________|       |_______________|       |___________|

la cola unicamente puede enviar datos de un solo tipo. es decir,  si
se crea una cola donde el primer dato que se envia es un entero,
los siguientes datos que esten en la cola solo pueden ser enteros.

hay que tener en cuenta que las colas son finitas, es decir, aunque
se pueden enviar infinitos datos, solo pueden tener guardados en la cola
una cantidad de datos limitada.

tambien es posible crear una estructura de datos custom y hacer una cola
de estas estructuras.

la idea basica es que se pueden enviar mensajes de una tarea a la otra
de una forma ordenada.

De una forma mas tecnica, supongase que tiene una tarea llamada TaskR que 
se encarga de trabajar con las comunicaciones (lectura de I2C, SPI, etc..)
la idea es que esa tarea se encargue simplemente de conseguir la data.
Luego se tiene otra tarea llamada TaskG que se encarga de procesar los
datos.

para este caso es conveniente crear una cola ya que la velocidad de trabajo
para leer los datos y procesarlos son diferentes, asi que cada trama
de datos que llegan en la TaskR se envian a la cola y apenas esta tenga
un dato, la tarea G puede seguir trabajando.

supongase que la TaskR lee datos cada 200 milisegundos y tiene 8 sleeps
de 7000ms; y la TaskG procesa datos cada 1000ms. En este caso el trabajo
no puede ser sincrono. Para este caso se puede crear una cola que acumule
esos datos de la TaskR que la TaskG no alcance a procesar.  

hay que tener en cuenta que si la TaskR tiene una velocidad mucho mas
alta que la TaskG se van a acumular mas datos que los que soporta la cola
por lo que esta estara llena y surgirán errores por lo que se debe poner
un Sleep de 7000ms cada 8 lecturas.
*/

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/queue.h" //libreria de colas

 
#define ledB 10
#define ledY 11
#define STACK_SIZE 1024*2 
#define R_DELAY 200
#define G_DELAY 1000

/*
El siguiente tipo de dato almacena la cola y sirve como un identificador
de la misma. Este se inicializa en cero de la siguiente forma:
*/
QueueHandle_t colaGlobal = 0;

const char *tag = "main";

esp_err_t init_led(void);
esp_err_t create_tasks(void); 

void vTaskR(void * pvParameters); 
void vTaskG(void * pvParameters);

void app_main(void)
{
    /*
    A continuacion se le asigna una cola a la variable "colaGolbal"
    para ello se le asigna el objeto xQueueCreate() el cual tiene los
    siguientes parametros:

    1) el tamaño de la cola: en el ejemplo se pone 20 posiciones
    2) el tamaño del tipo de dato que trabaja la cola: en el ejemplo}
       se trabajará con el tamaño de un dato entero de 32 bits.
    */
    colaGlobal = xQueueCreate(20, sizeof(uint32_t));
    init_led();
    create_tasks(); 

}
 
esp_err_t init_led()
{
    gpio_reset_pin(ledB);
    gpio_set_direction(ledB, GPIO_MODE_OUTPUT);
    gpio_reset_pin(ledY);
    gpio_set_direction(ledY, GPIO_MODE_OUTPUT);
    return ESP_OK;
}
esp_err_t create_tasks(void){

    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;

    xTaskCreate(vTaskR,
                "vTaskR",
                STACK_SIZE,
                &ucParameterToPass, 
                1,
                &xHandle);  
    xTaskCreate(vTaskG,
                "vTaskG",
                STACK_SIZE,
                &ucParameterToPass,
                1,
                &xHandle);
    return ESP_OK;
}

void vTaskR (void * pvParameters)
{
    while (1)
    {
        for (size_t i = 0; i < 8; i++)
        {
            /*
            a continuacion se envian datos por medio de la cola usando el metodo
            xQueueSend() el cual tiene los siguientes parametros:

            1) el handle de la cola: para este ejemplo es "colaGlobal".
            2) un puntero a una variable que se enviará a la cola: para este caso
            es la variable contadora "i".
            3) el tiempo que se va a estar esperando si la cola esta llena: en este
            se va a establecer en 100ms.
            
            linea de codigo:
            xQueueSend(colaGlobal, &i, pdMS_TO_TICKS(100));

            esto puede crearse así. sin embargo para ser mas organizados se va a
            crear un condicional que evalue si no se pudo enviar la cola. hay
            que tener en cuenta que el metodo xQueueSend() devuelve 0 si se envia
            correctamente el dato a la cola; en caso contrario devuelve 1.

            por ende, para manejar codigos de error se escribe:
            */
            vTaskDelay(pdMS_TO_TICKS(R_DELAY/2));
            gpio_set_level(ledB,1); // se enciende un led para visualizar el envio de datos
            ESP_LOGW(tag, "enviando %i a la cola", i);

            if(!xQueueSend(colaGlobal,&i,pdMS_TO_TICKS(100)))
            {
                ESP_LOGE(tag,"Error enviando %i a la cola", i);
            }
            
            vTaskDelay(pdMS_TO_TICKS(R_DELAY/2));

            gpio_set_level(ledB,0);
        }
        /*
        se pone un tiempo de espera de 7000ms para darle tiempo a la TaskG 
        para procesar los datos.
        */
        vTaskDelay(pdMS_TO_TICKS(7000));
    }
}

void vTaskG (void * pvParameters)
{
    int valorRecibido = 0;

    while (1)
    {
        /*
        a continuacion se va a recibir datos de la cola usando el metodo 
        xQueueReceive() el cual tiene los siguientes parametros:

        1) el handle de la cola: para este ejemplo es "colaGlobal".
        2) un puntero a la variable que recibirá la cola: para este caso es 
        "valorRecibido".
        3) el tiempo que se va a estar esperando si no hay ningun elemento en
        la cola: para este caso se va a establecer en 100ms.
        
        linea de codigo:
        xQueueReceive(colaGlobal, *valorRecibido,pdMS_TO_TICKS(100));
        
        esto puede crearse así. sin embargo para ser mas organizados se va a
        crear un condicional que evalue si no se recibio la cola. hay
        que tener en cuenta que el metodo xQueueReceive() devuelve 0 si se recibe
        correctamente el dato a la cola; en caso contrario devuelve 1.
        */

        if(!xQueueReceive(colaGlobal,&valorRecibido,pdMS_TO_TICKS(100)))
        {
            ESP_LOGE(tag,"Error recibiendo datos de la cola");
        }
        else
        {
            /*
            Si se recibe un dato de la cola se enciende y apaga un led con un
            intervalo de 100ms
            */
            vTaskDelay(pdMS_TO_TICKS(G_DELAY/2));
            gpio_set_level(ledY,1);
            
            ESP_LOGI(tag, "Valor recibido de la cola: %i", valorRecibido);
            
            vTaskDelay(pdMS_TO_TICKS(G_DELAY/2));
            
            gpio_set_level(ledY, 0);
        }
        /*
        se pone un tiempo de espera para evitar que se active el watchdog.
        */
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}
