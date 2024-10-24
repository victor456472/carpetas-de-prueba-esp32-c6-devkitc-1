/*
SEMAFOROS BINARIOS

permiten comunicar o dar instruccion desde una tarea a otra para que esta
segunda empiece a trabajar. dicha segunda tarea empezará dormida y esperara
a que otra tarea le de una orden con el semaforo:

     _____________
    |             |
    |    TaskR    |
    |_____________|
           |
           |
           V
     _____________
    |             |
    |   Semaforo  |
    |   Binario   |
    |_____________|
           |
           |
           V
     _____________
    |             |
    |    TaskG    |
    |_____________|

a diferencia de los semaforos reales que tienen 3 estados (rojo, amarillo y)
verde, los semaforos binarios solo tienen dos estados: activo, dormido.
dependiendo de esos estados la TaskG se va a ejecutar o va a permanecer
inactiva sin consumir CPU.

para codificar un semaforo binario se hace algo muy similar a lo que se 
hace en un MUTEX, sin embargo, la diferencia radica en que primero TaskR da
la llave y luego TaskG la recibe permitiendo asi que se ejecute la tarea.

     ____________________
    |                    |
    |  Create Semaphore  |
    |____________________|
             |
             |       ________________
             |      |                |
             |----->|    Give Key    |
                    |________________|
                             |
                             |       ________________
                             |      |                |
                             |----->|   Take key     |
                                    |________________|
                                            |
                                            |    ________________
                                            |   |                |
                                            |-->|  Run process   |
                                                |________________|




*/

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/semphr.h" 

 
#define ledB 10
#define ledY 11
#define STACK_SIZE 1024*2 
#define R_DELAY 10000
#define G_DELAY 100

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
    LlaveGlobal = xSemaphoreCreateBinary(); // se crea el semaforo binario
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

/*
vTaskR va a encender y apagar 8 veces el led azul y luego de encenderlo y apagarlo 
se va a entregar la llave.
*/

void vTaskR (void * pvParameters)
{
    while (1)
    {
        for (size_t i = 0; i < 8; i++)
        {
            vTaskDelay(pdMS_TO_TICKS(400));
            gpio_set_level(ledB,1);
            vTaskDelay(pdMS_TO_TICKS(400));
            gpio_set_level(ledB,0);
        }
        ESP_LOGI(tag,"TaskR entrega la llave al semaforo binario");

        /*
        con el metodo xSemaphoreGive() se entrega la llave al semaforo binario
        */

        xSemaphoreGive(LlaveGlobal);
        vTaskDelay(pdMS_TO_TICKS(R_DELAY));
    }
}

void vTaskG (void * pvParameters)
{
    while (1)
    {
        /*
        ahora el vTaskG debe esperar la llave durante un tiempo elevado y 
        cuando este disponible tomarla. para esto se usa el metodo
        xSemaphoreTake() que tiene dos parametros:

        1) la llave del semaforo
        2) el tiempo que va a tardar esperando

        como conviene poner un tiempo elevado, se usa el atributo porMAX_DELAY
        que establece un tiempo de hasta 12 horas.
        */
        if(xSemaphoreTake(LlaveGlobal, portMAX_DELAY))
        {
            ESP_LOGI(tag, "TaskG esta trabajando");

            for (size_t i = 0; i < 8; i++)
            {
                vTaskDelay(pdMS_TO_TICKS(400));
                gpio_set_level(ledY,1);
                vTaskDelay(pdMS_TO_TICKS(400));
                gpio_set_level(ledY,0);
            }
            
            ESP_LOGW(tag, "TaskG esta durmiendo");
        }

        vTaskDelay(pdMS_TO_TICKS(G_DELAY));
    }
}
