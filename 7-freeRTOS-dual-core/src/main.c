/*
ADVERTENCIA: ESTA PRACTICA NO FUNCIONA CON LA ESP32-C6-DevKitC-1 ya que requiere de
un controlador con dos nucleos. 

En la practica de multitasking se mostró como se pueden tener varias tareas ejecutandose
en un solo nucleo del controlador. ahora se va a ver como se asigna un nucleo a un
determinado conjunto de tareas.

para hacer esto se hará uso de la funcion xTaskCreatePinnedToCore() la cual es muy 
similar a xTaskCreate. la diferencia es que este añade un parametro adicional que
es el identificador del nucleo.

antes de intentar esto debe cerciorarse que su microcontrolador tiene mas de un nucleo.
esto se hace invocando al atributo portNUM_PROCESSORS. puede escribir una linea
de codigo en el app_main() para hacer esta verificacion:

ESP_LOGI(tag, "Numero de nucleos: %i", portNUM_PROCESSORS);


*/


#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define ledB 10
#define ledY 11

#define STACK_SIZE 1024*2 

#define B_DELAY 100
#define Y_DELAY 1000

const char *tag = "main";

esp_err_t init_led(void);
esp_err_t create_tasks(void); 

void vTaskB(void * pvParameters);
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

    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;

    /*
    
    para crear una tarea asociada a un nucleo se usa el metodo 
    xTaskCreatePinnedToCore() el cual tiene 7 parametros:

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
    7) el identificador del nucleo que ejecutará la tarea. si a este
       parametro se le pasa el atributo tskKNO_AFFINITY, el programa
       eligirá por conveniencia el nucleo que ejecutara la tarea dependiendo
       de la disponibilidad de la memoria.    
    
    */

    xTaskCreatePinnedToCore(vTaskB,
                            "vTaskB",
                            STACK_SIZE,
                            &ucParameterToPass, // con & se crea un puntero que apunta a la variable ucParameterToPass.
                                                // los punteros devuelven una direccion de memoria y el dato de esa memoria
                            1,
                            &xHandle, // puntero que permite cambiar el estado del task
                            0); //nucleo #0
    xTaskCreatePinnedToCore(vTaskY,
                            "vTaskY",
                            STACK_SIZE,
                            &ucParameterToPass,
                            1,
                            &xHandle,
                            1); //nucleo #2
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
        ESP_LOGI(tag, "Led B - core 0");
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
        ESP_LOGI(tag, "Led Y - core 1");
        gpio_set_level(ledY,1);
        vTaskDelay(pdMS_TO_TICKS(Y_DELAY));
        gpio_set_level(ledY,0);
        vTaskDelay(pdMS_TO_TICKS(Y_DELAY)); 
    }
    
}