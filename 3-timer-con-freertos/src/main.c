#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/timers.h" //libreria de timers con eventos de freertos

#define led1 8

static const char *TAG = "main";
uint8_t led_level = 0;

esp_err_t init_led(void);
esp_err_t blink_led(void);
esp_err_t set_timer(void); // establecer timer

/*variables asociadas al  timer con eventos de  freertos*/

TimerHandle_t xTimers; //se usa para guardar respuestas del timer de freertos
int interval = 1000; //periodo de espera del timer
int timerId = 1; //identificador unico del timer

//la siguiente funcion es llamada por el timer de freertos cada cierto intervalo de tiempo:

void vTimerCallback(TimerHandle_t pxTimer)
{
    ESP_LOGI(TAG, "Event was called from timer 1");
    blink_led();
}

void app_main(void)
{

    init_led(); 
    set_timer(); //establecer timer
}

esp_err_t init_led(void)
{
    gpio_reset_pin(led1);
    gpio_set_direction(led1, GPIO_MODE_OUTPUT);
    return ESP_OK;
}
esp_err_t blink_led(void)
{
    led_level = !led_level;
    gpio_set_level(led1, led_level);
    return ESP_OK;
}

//se desarrolla el codigo de la funcion set_timer()
esp_err_t set_timer(void)
{
    ESP_LOGI(TAG, "Timer init configuration");

    /*
    hipotesis a corroborar: un timer permite ejecutar tareas cada cierto intervalo de tiempo incluso si se esta ejecutando un loop en el main
    
    descripcion:

    los timers se declaran mediante la funcion xTimerCreate() la cual tiene los siguientes parametros:
    1- Nombte del timer: puede ser arbitrario
    2- periodo de ejecucion en Ticks: es el tiempo que tarda el timer en ejecutar una tarea. debe usarse
       la funcion pdMS_TO_TICKS para ingresar el periodo de tiempo en ms de forma que esta automaticamente 
       lo convierta a Ticks de reloj.
    3- Hipotesis: El tercer parametro permite establecer si se quiere que el timer se reinicie cuando finalice la ejecucion
       de la tarea. si se establece en pdTRUE, la tarea se volvera a repetir infinitamente luego de un periodo de tiempo.
       en cambio, si se establece en pdFALSE el timer esperara un periodo de tiempo, ejecutará la tarea y luego se va a
       detener.
    4- timer  id: es un identificador unico para el timer
    5- funcion que llama el timer una vez expira para ejecutar una tarea
    */
    xTimers = xTimerCreate("Timer",         
                           (pdMS_TO_TICKS(interval)), 
                           pdTRUE,          
                           (void *)timerId,   
                           vTimerCallback 
    );

    /*
    
    El siguiente algoritmo identifica cuando ocurre un error al inicializar el timer por medio de la
    variable xTimers. En caso dado muestra un error en pantalla:
    
    */

    if (xTimers == NULL)
    {
        // el timer no se creó porque xTimers == NULL
        ESP_LOGE(TAG, "The timer was not created.");
    }
    else
    {

        if (xTimerStart(xTimers, 0) != pdPASS)
        {
            // El timer se creó pero no alcanzó a estar en modo activo.
            ESP_LOGE(TAG, "The timer could not be set into the Active state.");
        }
    }
    return ESP_OK;
}
