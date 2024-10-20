#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include "driver/ledc.h" // libreria para PWM

static const char *TAG = "main";
uint8_t led_level = 0;

esp_err_t set_timer(void);
esp_err_t set_pwm(void);      // metodo para configurar pines PWM
esp_err_t set_pwm_duty(void); // metodo para establecer el duty cicle del pwm

/*timers*/

TimerHandle_t xTimers;
int interval = 10;
int timerId = 1;

/*duty variables*/
int dutyB = 0;   // inicializa el led azul apagado
int dutyY = 500; // inicializa el led amarillo a mitad de brillo

void vTimerCallback(TimerHandle_t pxTimer)
{
    dutyB += 5;
    if (dutyB > 1023) // 1023
        dutyB = 0;

    dutyY += 5;
    if (dutyY > 1023) // 1023
        dutyY = 0;

    set_pwm_duty();
}

void app_main(void)
{
    set_pwm();
    set_timer();
}

esp_err_t set_timer(void)
{
    ESP_LOGI(TAG, "Timer init configuration");
    xTimers = xTimerCreate("Timer",                   
                           (pdMS_TO_TICKS(interval)), 
                           pdTRUE,                   
                           (void *)timerId,           
                           vTimerCallback             
    );

    if (xTimers == NULL)
    {
        ESP_LOGE(TAG, "The timer was not created.");
    }
    else
    {

        if (xTimerStart(xTimers, 0) != pdPASS)
        {
            ESP_LOGE(TAG, "The timer could not be set into the Active state.");
        }
    }
    return ESP_OK;
}
esp_err_t set_pwm(void)
{
    /*

    Establecimiento de propiedades de los canales:

    para poder configurar un pin PWM hay que declarar una estructura llamada ledc_channel_config_t la cual
    tiene 6 parametros:

    1- el gpio que se usará como PWM, al igual que arduino hay que revisar que GPIOs admiten PWM.

    2- el tipo de velocidad del PWM. para algunas ESP32 esta la opcion de alta velocidad sin embargo para el caso
       del ESP32-C6-DevKitC-1 unicamente se puede utilizar la opcion de "baja velocidad" la cual sigue siendo bastante
       alta respecto a un arduino. por ejemplo el PWM mas alto por defecto para arduino UNO tiene una frecuencia de
       980Hz y una resolución de 8bits en los pines 5 y 6. en cambio el ESP32 puede alcanzar frecuencias de 78.13kHz
       con una resolucion de 10 bits (lo que implica que el duty cicle va de 0 a 1023)

       claramente con el ESP32-C6-DevKitC-1 se puede alcanzar resoluciones de hasta 20 bits (duty cicle de 0 a 1048575)
       sin embargo esto hace que la frecuencia disminuya a un maximo de 76.29Hz (se puede usar hasta 16 bits con frecuencias
       de hasta 1.22kHz)

    3- canal del pin PWM: este se puede escoger arbitrariamente entre 0 y 6 para el ESP32-C6-DevKitC-1. no debe escojer el
       mismo canal para dos GPIO distintos.

    4- tipo de interrupción. para este ejemplo no se va a utilizar interrupciones por lo que se deja en DISABLE.

    5- definir timer: El ESP32 tiene internamente varios timers que pueden usarse para cada pin PWM. estos pueden
       incluso configurarse para ajustar diversas frecuencias y resoluciones. sin embargo en este ejemplo
       solo se usa un timer. tambien se recuerda que un timer es un circuito integrado que genera señales periodicas cuadradas
       y este se suele estudiar en electronica analoga y digital.

    6- duty cicle: es el duty cicle de la señal PWM por defecto. se suele configurar en cero.

    */
    ledc_channel_config_t channelConfigB = {0};
    channelConfigB.gpio_num = 8;
    channelConfigB.speed_mode = LEDC_LOW_SPEED_MODE;
    channelConfigB.channel = LEDC_CHANNEL_0;
    channelConfigB.intr_type = LEDC_INTR_DISABLE;
    channelConfigB.timer_sel = LEDC_TIMER_0;
    channelConfigB.duty = 0;

    ledc_channel_config_t channelConfigY = {0};
    channelConfigY.gpio_num = 10;
    channelConfigY.speed_mode = LEDC_LOW_SPEED_MODE;
    channelConfigY.channel = LEDC_CHANNEL_1;
    channelConfigY.intr_type = LEDC_INTR_DISABLE;
    channelConfigY.timer_sel = LEDC_TIMER_0;
    channelConfigY.duty = 0;

    /*
    una vez se declara la estructura de los canales se configuran mediante la funcion
    ledc_channel_config()
    */

    ledc_channel_config(&channelConfigB);
    ledc_channel_config(&channelConfigY);

    /*

    Establecimiento de propiedades del timer:

    para poder configurar los timers de los pines PWM es necesario declarar una estructura llamada ledc_timer_config_t
    la cual tiene 4 parametros:

    1- el tipo de velocidad del PWM. para algunas ESP32 esta la opcion de alta velocidad sin embargo para el caso
       del ESP32-C6-DevKitC-1 unicamente se puede utilizar la opcion de "baja velocidad" la cual sigue siendo bastante
       alta respecto a un arduino. por ejemplo el PWM mas alto por defecto para arduino UNO tiene una frecuencia de
       980Hz y una resolución de 8bits en los pines 5 y 6. en cambio el ESP32 puede alcanzar frecuencias de 78.13kHz
       con una resolucion de 10 bits (lo que implica que el duty cicle va de 0 a 1023)

       claramente con el ESP32-C6-DevKitC-1 se puede alcanzar resoluciones de hasta 20 bits (duty cicle de 0 a 1048575)
       sin embargo esto hace que la frecuencia disminuya a un maximo de 76.29Hz (se puede usar hasta 16 bits con frecuencias
       de hasta 1.22kHz)

    2- La resolución del timer: esta indica el numero de bits que se usaran para modificar el ancho de banda de la señal
       PWM. A mayor numero de bits habrá una mayor resolución (ideal para aplicaciones de microseñales y control robusto)
       sin embargo disminuira la freccuencia de la señal PWM.

       para el caso de la ESP32-C6-DevKitC-1 se puede alcanzar una resolucion maxima de 20bits sin embargo lo comun es
       usar 10 bits.

       el numero de bits indica cual sera el valor maximo que puede tener el duty cicle. este se calcula de esta forma:

       max_duty=[2^(#bits)]-1

       asi, por ejemplo, para un arduino que tiene un timer de 8 bits, la señal PWM puede tener un duty cicle de
       [2^(8)]-1 = 255 ; en cambio un timer de ESP32-C6-DevKitC-1 puede llegar a tener un duty cicle de hasta
       [2^(10)]-1 = 1023.

    3- definir timer: El ESP32 tiene internamente varios timers que pueden usarse para cada pin PWM. estos pueden
       incluso configurarse para ajustar diversas frecuencias y resoluciones. sin embargo en este ejemplo
       solo se usa un timer. tambien se recuerda que un timer es un circuito integrado que genera señales periodicas cuadradas
       y este se suele estudiar en electronica analoga y digital.

    4- Frecuencia del timer en Hz. para el ESP32-C6-DevKitC-1 los timers configurados en LOW_SPEED_MOD tienen las siguientes
       frecuencias maximas:

        | Resolución (bits) | Frecuencia máxima (Hz) |
        |-------------------|------------------------|
        | 1 bit             | 40 MHz                 |
        | 2 bits            | 20 MHz                 |
        | 8 bits            | 312.5 kHz              |
        | 10 bits           | 78.13 kHz              |
        | 12 bits           | 19.53 kHz              |
        | 16 bits           | 1.22 kHz               |
        | 20 bits           | 76.29 Hz               |

    */

    ledc_timer_config_t timerConfig = {0};
    timerConfig.speed_mode = LEDC_LOW_SPEED_MODE;
    timerConfig.duty_resolution = LEDC_TIMER_10_BIT;
    timerConfig.timer_num = LEDC_TIMER_0;
    timerConfig.freq_hz = 1500; // 1500Hz

    /*

    Una vez se crea la estructura, se puede configurar el timer mediante el comando ledc_timer_config().
    hay que tener en cuenta que el parametro de entrada timerConfig tiene un "&" indicando que se esta
    creando un puntero hacia esa estructura.

    */

    ledc_timer_config(&timerConfig);
    return ESP_OK;
}
esp_err_t set_pwm_duty(void)
{

    /*

    Se establecen los duty cicles con las variables dutyB y dutyY a traves del comando ledc_set_duty.
    los parametros necesarios son:

    1- tipo de velocidad del PWM
    2- canal
    3- variable que almacena el valor de duty que se quiere establecer

    */

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, dutyB); // para led azul
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, dutyY); // para led amarillo

    /*

    con lo anterior se hace setting de los duty cicles pero para aplicar los cambios
    en los canales se debe usar la función ledc_update_duty.

    */

    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0); // para led azul
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1); // para led amarillo
    return ESP_OK;
}