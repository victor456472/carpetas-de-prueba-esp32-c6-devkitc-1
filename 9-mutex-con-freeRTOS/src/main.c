/*
FreeRTOS MUTEX (Exclusion mutua)
     ____________          ____________          ____________
    |            |        |            |        |            |
    |   TaskR    |------->|   MUTEX    |<-------|   TaskG    |
    |____________|        |____________|        |____________|
                                 |
                                 |
                                 V 
                       ____________________
                      |                    |
                      | recurso compartido |
                      |____________________|

las MUTEX son exclusiones mutuas que hacen que un recurso solo pueda 
compartirse con una tarea a la vez. este recurso puede ser una funcion,
una variable, etc...

si una TaskR esta usando el recurso la TaskG no puede acceder al recurso
compartido hasta que TaskG se desocupe. el MUTEX le dara una llave a
cada TASK para poder acceder al recurso compartido, por ejemplo, la 
TaskR toma esta llave y luego utiliza el recurso compartido. mmientras
TaskR tenga la llave, TaskG no puede acceder al recurso compartido. una
vez la TaskR entregue la llave, la TaskG puede acceder al recurso.

de forma grafica, los MUTEX se usan de esta forma:
     ________________
    |                |
    |  Create Mutex  |
    |________________|
             |
             |       ________________
             |      |                |
             |----->|    Take Key    |
                    |________________|
                             |
                             |       ________________
                             |      |                |
                             |----->|  Use shared    |
                                    |   Resource     |
                                    |________________|
                                            |
                                            |    ________________
                                            |   |                |
                                            |-->|    Give Key    |
                                                |________________|

A esta llave (key en inglés) se le conoce como semaforo.

*/

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/semphr.h" //libreria de semaforos

 
#define ledB 10
#define ledY 11
#define STACK_SIZE 1024*2 
#define R_DELAY 1000
#define G_DELAY 2000

const char *tag = "main";

esp_err_t init_led(void);
esp_err_t create_tasks(void); 
esp_err_t shared_resource(int led); //este metodo será el recurso compartido

void vTaskR(void * pvParameters); 
void vTaskG(void * pvParameters); 

//PARA EL MUTEX:

  SemaphoreHandle_t LlaveGlobal; //llave

void app_main(void)
{
    LlaveGlobal = xSemaphoreCreateMutex(); // se crea el Mutex
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

/*
el siguiente metodo va a encender o apagar un led 8 veces y luego estará
disponible para ejecutarse nuevamente. este metodo sera nuetro recurso
compartido.
*/
esp_err_t shared_resource(int led)
{
    for (size_t i = 0; i < 8; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(400));
        gpio_set_level(led,1);
        vTaskDelay(pdMS_TO_TICKS(400));
        gpio_set_level(led,0);
    }
    return ESP_OK;
}

void vTaskR (void * pvParameters)
{
    while (1)
    {
        /*
        para implementar el semaforo simplemente se necesita evaluar si la llave
        fue tomada o no por alguna tarea. para ello se usa el metodo xSemaphoreTake()
        el cual tiene dos parametros:

        1) la llave del MUTEX
        2) el tiempo de espera para recibir la llave.

        el metodo xSemaphoreTake() devuelve 1 si se recibe la llave. En caso contrario
        devuelve cero. con esto se puede crear un condicional que ejecute una
        tarea determinada en funcion de si se tomó la llave o no.
        */
        if(xSemaphoreTake(LlaveGlobal, pdMS_TO_TICKS(100)))
        {
            ESP_LOGI(tag, "TaskR tomo el recurso compartido");
            shared_resource(ledB);
            xSemaphoreGive(LlaveGlobal); // ejecutada la tarea se vuelve a entregar la llave.
        }
        /*
        se pone un tiempo de espera a la tarea. entre mas corto sea el tiempo de
        espera, mas probable es que la tarea agarre primero la llave.
        */
        vTaskDelay(pdMS_TO_TICKS(R_DELAY));
    }
}

void vTaskG (void * pvParameters)
{
    while (1)
    {
        if(xSemaphoreTake(LlaveGlobal, pdMS_TO_TICKS(100)))
        {
            ESP_LOGI(tag, "TaskG tomo el recurso compartido");
            shared_resource(ledY);
            xSemaphoreGive(LlaveGlobal);
        }

        vTaskDelay(pdMS_TO_TICKS(G_DELAY));
    }
}
