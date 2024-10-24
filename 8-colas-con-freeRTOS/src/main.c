/*
COLAS

son el envio ordenado de datos de una tarea hacia otra, de modo que podamos
comunicarlas.
     ___________         _______________         ___________
    |           |       |               |       |           |
    |  tarea 1  | ----->|     COLA      |------>|  tarea 2  |
    |___________|       |_______________|       |___________|

la cola unicamente puede enviar datos de un solo tipo. es decir,  si
se crea una cola donde el primer dato que se envia es un entero,
los siguientes datos que esten en la cola solo pueden ser enteros.

hay que tener en cuenta que las colas son finitas, es decir, aunque
se pueden enviar infinitos datos, solo pueden tener guardados en la cola
una cantidad de datos limitada

continuar viendo: https://youtu.be/abwyjmfZ0mQ?si=k8Uh1BhdUh9m6cJQ&t=55
*/

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"



void app_main(void)
{
 
}
 
