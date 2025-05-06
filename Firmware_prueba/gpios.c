/*
Funciones para el control del LEDs, zumbador y los pulsadores.
*/
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h" //IRAM_ATTR: poner el código de la ISR del botón en RAM en lugar de FLASH.
#include "pinOut.h"
#include "gpios.h"

#define ESP_INTR_FLAG_DEFAULT 0
#define STACK_SIZE 2048 //Stack de las tareas.

enum estadoPIN {OFF, ON};

//Reserva en RAM del stack and task control block (TCB) para las tareas creadas estáticas
static StackType_t stack_tarea_gpios[STACK_SIZE];
static StaticTask_t TCB_tarea_gpios;
//Declaramos los handlers para las tasks estáticas por si queremos suspenderlas desde otras tasks.
TaskHandle_t xHandle_gpios_crear_tarea = NULL;

//Variable para la ISR de los botores.
volatile uint8_t gvui8_pulsacion = 0;
volatile uint32_t gvui32_contadorISR = 0;

//Función que asignamos a la tarea de los gpios
static void tarea_gpios(void *pvParameters);

//Declaración de función para inicializar el hardware de los LEDs y pulsadores.
static void init_hardware(void);

//Declaración de la función para crear la tarea que llamaremos desde main.c
void gpios_crear_tarea();

//Función que llamamos cuando ocurra la ISR del pulsador. RAM_ATTR: En RAM se ejecuta más rápido. 
//static void IRAM_ATTR pulsador_isr_handler(void *arg);
static void pulsador_isr_handler(void *arg); 

/******************Definición de las funciones*******************/

void gpios_crear_tarea(void)
{
    xHandle_gpios_crear_tarea = xTaskCreateStatic(
        &tarea_gpios,      //Función que se asigna a al tarea.
        "tareaGPIOs",       //Nombre que le asignamos a la tarea (debug). 
        STACK_SIZE,                 //Tamaño del stack de la tarea (reservado de manera estática previamente).
        NULL,                       //Parámetros pasados a la tarea en su creación.
        tskIDLE_PRIORITY + 1,       //Prioridad de la tarea. 
        stack_tarea_gpios,       //Array para el stack de la tarea reservado previamente. 
        &TCB_tarea_gpios);       //Memoria reservada previamente para el TCB (task control block) de la tarea.
    
        printf("Tarea para los LEDs, pulsadores y zumbador creada.\n");  
}

//Función que asignamos a la tarea de los gpios
static void tarea_gpios(void *pvParameters)
{
    //Inicializamos los pines del microcontrolador.
    init_hardware();

    //Activamos el zumbador al inicio
    gpio_set_level(ZUMBADOR, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS); 
    gpio_set_level(ZUMBADOR, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS); 
    gpio_set_level(ZUMBADOR, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS); 
    gpio_set_level(ZUMBADOR, 0);
    while (1)
    {
        if(gvui8_pulsacion)
        {
            gpio_set_level(ZUMBADOR, 1);
            vTaskDelay(250 / portTICK_PERIOD_MS);
            gpio_set_level(ZUMBADOR, 0);
            gvui8_pulsacion = 0;
            printf("Numero de veces que se han pulsado los pulsadores: %ld.\n", gvui32_contadorISR);
        }
        gpio_set_level(LED_AMARILLO, 1);
        gpio_set_level(LED_ROJO, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS); 
        gpio_set_level(LED_ROJO, 1);
        gpio_set_level(LED_AMARILLO, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);     
    }
}

//Definición de función para inicializar el hardware de los LEDs y pulsadores.
static void init_hardware(void)
{
    //Configuramos el pin del led rojo como salida.
    gpio_reset_pin(LED_ROJO);
    gpio_set_direction(LED_ROJO, GPIO_MODE_OUTPUT);
    gpio_pullup_dis(LED_ROJO);
    gpio_set_level(LED_ROJO, 1);
    //Configuramos el pin del led amarillo como salida.
    gpio_reset_pin(LED_AMARILLO);
    gpio_set_direction(LED_AMARILLO, GPIO_MODE_OUTPUT);
    gpio_pullup_dis(LED_AMARILLO);
    gpio_set_level(LED_AMARILLO, 1);
    //Configuramos el pin del zumbador como salida.
    gpio_reset_pin(ZUMBADOR);
    gpio_set_direction(ZUMBADOR, GPIO_MODE_OUTPUT);
    gpio_pullup_dis(ZUMBADOR);

    //Configuración de los pusaldores

    //Configuramos el pin del pulsador como entrada.
    gpio_reset_pin(PB_A);
    gpio_set_direction(PB_A, GPIO_MODE_INPUT);
    //Deshabilitamos la resistencia de pull-up del pulsador y usamos una externa.
    gpio_pullup_dis(PB_A);
    //Configurar la interrupción del pulsador.
    gpio_set_intr_type(PB_A, GPIO_INTR_NEGEDGE);
    //Instala el driver's GPIO ISR handler service.
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //Asignar la función a la que se llamará cuando ocurra la ISR del pulsador.
    gpio_isr_handler_add(PB_A, pulsador_isr_handler, NULL);

    //Configuramos el pin del pulsador como entrada.
    gpio_reset_pin(PB_B);
    gpio_set_direction(PB_B, GPIO_MODE_INPUT);
    //Deshabilitamos la resistencia de pull-up del pulsador y usamos una externa.
    gpio_pullup_dis(PB_B);
    //Configurar la interrupción del pulsador.
    gpio_set_intr_type(PB_B, GPIO_INTR_NEGEDGE);
    //Instala el driver's GPIO ISR handler service.
    //gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //Asignar la función a la que se llamará cuando ocurra la ISR del pulsador.
    gpio_isr_handler_add(PB_B, pulsador_isr_handler, NULL);

    //Configuramos el pin del pulsador como entrada.
    gpio_reset_pin(PB_C);
    gpio_set_direction(PB_C, GPIO_MODE_INPUT);
    //Deshabilitamos la resistencia de pull-up del pulsador y usamos una externa.
    gpio_pullup_dis(PB_C);
    //Configurar la interrupción del pulsador.
    gpio_set_intr_type(PB_C, GPIO_INTR_NEGEDGE);
    //Instala el driver's GPIO ISR handler service.
    //gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //Asignar la función a la que se llamará cuando ocurra la ISR del pulsador.
    gpio_isr_handler_add(PB_C, pulsador_isr_handler, NULL);
}

//Función que llamamos cuando ocurra la ISR del pulsador. RAM_ATTR: En RAM se ejecuta más rápido. 
//static void IRAM_ATTR pulsador_isr_handler(void *arg)
static void pulsador_isr_handler(void *arg)
{
    gvui8_pulsacion = 1;
    gvui32_contadorISR++;
}

