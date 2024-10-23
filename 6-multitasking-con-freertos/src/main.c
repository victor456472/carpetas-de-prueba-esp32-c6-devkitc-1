/*
En RTOS un programa se divide en diferentes funciones independientes 
(lo que se conoce como una tarea). estas funciones no se llaman en 
ninguna parte del programa, simplemente se crean. Cada tarea
se ejecuta continuamente, es decir, en bucle infinito.

algo interesante es que se pueden hacer tareas en paralelo, cada
una con su propio bucle infinito aun si su microcontrolador tiene
un solo nucleo.

las tareas pueden tener diferentes estados: Running, Ready, Blocked, 
Suspended

Colas o queues: las colas permiten enviar datos entre tareas de manera
que si esta aun se encuentra procesando algo, el dato quede en estado
de espera hasta que la tarea termine de procesar para aceptarlas. las
colas acumulan datos de forma que puedan ser procesados en orden 
mientras las tareas destino se preparan para procesarlos.

MUTEXES (Mutual Exclusion): es un semaforo que solamente deja que un
recurso se aproveche exclusivamente por una tarea a la vez.

BINARY SEMAPHORES: es una cola de un solo elemento en el cual no
interesa el tipo de dato en la cola sino la interrupcion. este tipo
de semaforo permite hacer que una tarea este  durmiendo para
que no consuma recursos del CPU mientras otra se encarga de mandarle
una interrupcion para avisarle que ejecute la tarea.

*/
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define ledB 10
#define ledY 11

#define STACK_SIZE 1024*2 //cantidad de memoria usada para los tasks

#define B_DELAY 100
#define Y_DELAY 1000

const char *tag = "main";

esp_err_t init_led(void);
esp_err_t create_tasks(void); //metodo para crear tasks 

void vTaskB(void * pvParameters); //funciones invocadas por tasks
void vTaskY(void * pvParameters);

void app_main(void)
{
    init_led();
    create_tasks(); 
    while(1)
    {
        printf("hello from main \n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
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
    /*
    El siguiente es un parametro que se puede pasar desde
    el main hasta la funccion que ejecuta el task. este parametro
    puede usarse para cualquier proposito, como por ejemplo
    para acceder al valor de un sensor que se lee en el main().

    hay que tener en cuenta que como este esta fuera de la funcion
    que ejecuta el task, debe ser referenciado mediante un puntero
    durante la creacion del task para que la funcion que ejecuta el
    task tenga acceso a la direccion de memoria asignada a
    ucParameterToPass.
    */
    static uint8_t ucParameterToPass;
    /*
    la siguiente variable permite controlar el estado de los tasks.
    sirve como un identificador dinamico del task y con este se
    puede configurar que un task se ponga en estado suspended,
    blocked, etc...

    por ejemplo, escribiendo vTaskSuspend(xHandle); se suspenden
    ambas tareas. 

    */
    TaskHandle_t xHandle = NULL;

    /*
    
    para crear una tarea se usa el metodo xTaskCreate() el cual tiene
    6 parametros:

    1) el metodo que ejecutará el task
    2) el nombre identificador del task. este puede ser escogido
       arbitrariamente y unicamente sirve para debugging.
    3) stackDepth: la cantidad de memoria que se usará para ejecutar
       la tarea.
    4) un parametro que se le puede pasar al metodo que ejecuta el
       task. Es sugerible que sea un puntero que apunte a una variable
       dentro de la funcion donde se esta creando el task
    5) el nivel de prioridad del task
    6) la variable que maneja el estado del task. tambien debe ser un 
       puntero.    
    
    */

    xTaskCreate(vTaskB,
                "vTaskB",
                STACK_SIZE,
                &ucParameterToPass, // con & se crea un puntero que apunta a la variable ucParameterToPass.
                                    // los punteros devuelven una direccion de memoria y el dato de esa memoria
                1,
                &xHandle);  //puntero que permite cambiar el estado del task
    xTaskCreate(vTaskY,
                "vTaskY",
                STACK_SIZE,
                &ucParameterToPass,
                1,
                &xHandle);
    return ESP_OK;
}

/*

En el siguiente metodo, la variable pvParameters almacena el puntero
&ucParameterToPass. debe declararse como void * indicando que lo
que se recibe puede ser un puntero de cualquier tipo de dato: entero
florante, char, etc...

algo a tener en cuenta es que si se invoca pvParameters este solo devolvera
la direccion de la memoria de esa variable en hexadecimal. sin embargo, 
se puede extraer el dato contenido en la memoria escribiendo un * antes
del nnombre de la variable: *pvParameters. a esto se le llama dereferenciación

*/

void vTaskB (void * pvParameters)
{
    while (1)
    {
        ESP_LOGI(tag, "Led B");
        gpio_set_level(ledB,1);                 //encender un led
        vTaskDelay(pdMS_TO_TICKS(B_DELAY));     //esperar un tiempo
        gpio_set_level(ledB,0);                 //apagar un led
        vTaskDelay(pdMS_TO_TICKS(B_DELAY)); 
    }
    
}

// task #2

void vTaskY (void * pvParameters)
{
    while (1)
    {
        ESP_LOGI(tag, "Led Y");
        gpio_set_level(ledY,1);
        vTaskDelay(pdMS_TO_TICKS(Y_DELAY));
        gpio_set_level(ledY,0);
        vTaskDelay(pdMS_TO_TICKS(Y_DELAY)); 
    }
    
}