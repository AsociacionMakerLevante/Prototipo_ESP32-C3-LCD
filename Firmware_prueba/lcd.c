/*
Funciones para controlar la pantalla LCD
*/
#include <stdio.h>
#include "driver/gpio.h"
#include "lcd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "pinOut.h"

#define STACK_SIZE 2048 //Stack de las tareas.

enum estadoPIN {OFF, ON}; 
enum LCD_Comando{VCOM = 0x00, LIMPIAR = 0x04, ESCRIBIR = 0x01};

//Reserva en RAM del stack and task control block (TCB) para las tareas creadas estáticas
static StackType_t stack_tarea_LCD[STACK_SIZE];
static StaticTask_t TCB_tarea_LCD;
//Declaramos los handlers para las tasks estáticas por si queremos suspenderlas desde otras tasks.
TaskHandle_t xHandle_lcd_crear_tarea = NULL;

//Dispositivo SPI que vamos a controlar.
spi_device_handle_t spi_device_LCD;

//Función que asignamos a la tarea del LCD.
static void lcd_LCD(void *pvParameters);

//Función para conmutar LCD EXTCON
static void lcd_extcon(enum estadoPIN estado);

//Funciones para la transmisión SPI.
static void lcd_spi_init(void);
static void lcd_spi_escribir_linea(uint8_t linea, uint8_t *data);
static void lcd_limpiar_pantalla();

/**************Definición de las funciones ***********************/

//Definición de la función que crea la tarea para controlar el LCD.
void lcd_crear_tarea_LCD(void)
{
    xHandle_lcd_crear_tarea = xTaskCreateStatic(
        &lcd_LCD,              //Función que se asigna a al tarea.
        "tarea_LCD",           //Nombre que le asignamos a la tarea (debug). 
        STACK_SIZE,            //Tamaño del stack de la tarea (reservado de manera estática previamente).
        NULL,                  //Parámetros pasados a la tarea en su creación.
        tskIDLE_PRIORITY + 2,  //Prioridad de la tarea. 
        stack_tarea_LCD,       //Array para el stack de la tarea reservado previamente. 
        &TCB_tarea_LCD);       //Memoria reservada previamente para el TCB (task control block) de la tarea.
    
        printf("Tarea para controlar el LCD creada.\n");  
}

//Función que asignamos a la tarea del LCD.
static void lcd_LCD(void *pvParameters)
{
    TickType_t xLastWakeTime;
    enum estadoPIN estado = OFF;

    // Inicializa xLastWakeTime con el tiempo actual.
    xLastWakeTime = xTaskGetTickCount();

    //Inicializamos el hardware conectado al LCD.
    lcd_spi_init();
    lcd_limpiar_pantalla();

    //Línea que se va a escribir.
    uint8_t linea = 127;    
    //contenido de la línea. (en el stack de la tarea)
    uint8_t  data_linea[52] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00 };

    while(1)
    {
        //InvertimosEXTCON
        estado = !estado;
        lcd_extcon(estado);

        if(estado == true)
        {
            //Limpiamos la pantalla.
            lcd_limpiar_pantalla();
            //Escribimos líneas en la pantalla.
            lcd_spi_escribir_linea(linea, data_linea);
            lcd_spi_escribir_linea((linea + 4), &data_linea[0]);
            lcd_spi_escribir_linea((linea + 8), data_linea);
            lcd_spi_escribir_linea((linea + 12), data_linea);
        }
        else
        {
            //Limpiamos la pantalla.
            lcd_limpiar_pantalla();
            //Escribimos líneas en la pantalla
            lcd_spi_escribir_linea((linea + 16), data_linea);
            lcd_spi_escribir_linea((linea + 20), data_linea);
            lcd_spi_escribir_linea((linea + 24), data_linea);
            lcd_spi_escribir_linea((linea + 28), data_linea);
        }
        //Bloqueamos la tarea durante 500 ms
        vTaskDelayUntil( &xLastWakeTime, (500 / portTICK_PERIOD_MS) );
    }
}

//Funcion para conmutar LCD EXTCON
static void lcd_extcon(enum estadoPIN estado)
{
    gpio_set_level(LCD_EXTCON, estado);
}

//Declaración de las funciones para la transmisión SPI.
static void lcd_spi_init(void)
{
   //Configuramos el pin de LCD EXTCON como salida.
   gpio_reset_pin(LCD_EXTCON);
   gpio_set_direction(LCD_EXTCON, GPIO_MODE_OUTPUT);
   gpio_pullup_dis(LCD_EXTCON);
   gpio_set_level(LCD_EXTCON, 0);

    //Inicializar el bus SPI llamando a spi_bus_initialize().
    //Estructura de inicialización spi_bus_config_t.
    spi_bus_config_t conf_bus_SPI = {
        .mosi_io_num = SPI_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .data_io_default_level = 0,
        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
        .flags = SPI_DEVICE_POSITIVE_CS,
    };
    spi_bus_initialize(SPI2_HOST, &conf_bus_SPI, SPI_DMA_DISABLED);

    //Registrar el dispositivo LCD en el bus SPI llamando a spi_bus_add_device();
    //Estrucutura spi_device_interface_config_t.
    spi_device_interface_config_t conf_interfaz_SPI =
    {
        .command_bits = 8,
        .address_bits = 8,
        .dummy_bits = 0,
        .mode = 0, 
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = 2000000UL,
        .input_delay_ns = 0,
        .spics_io_num = SPI_CS,
        .flags = (SPI_DEVICE_POSITIVE_CS | SPI_DEVICE_TXBIT_LSBFIRST),
        .queue_size = 1,
        //.pre_cb = NULL,
        //.post_cb = NULL,
    };
    spi_bus_add_device(SPI2_HOST, &conf_interfaz_SPI, &spi_device_LCD);

    //Ponemos a 1 EXTCOM
    gpio_set_level(LCD_EXTCON, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS); 
}
static void lcd_spi_escribir_linea(uint8_t linea, uint8_t *data)
{
    enum LCD_Comando comando = ESCRIBIR;

    //Para interactura con el dispositivo rellenar la estructura spi_transaction_t.
    //funciones spi_device_queue_trans()  o spi_device_polling_transmit() 
    //Escribimos una línea. El LCD recibe el LSB primero
    spi_transaction_t spi_envio_LCD =
    {
        //.flags = NULL,
        .cmd = comando,
        .addr = linea,
        .length = 416,
        .rxlength = 0,
        .user = NULL,
        .tx_buffer = data, //no poner &data, si no el contenido, la dirección a la que apunta data.
        .rx_buffer = NULL,
    };
    spi_device_polling_transmit(spi_device_LCD, &spi_envio_LCD); 
}
static void lcd_limpiar_pantalla()
{
    //Limpiar pantalla.

    enum LCD_Comando comando = LIMPIAR ;

    spi_transaction_t spi_envio_LCD_CS =
    {
        //.flags = NULL,
        .cmd = comando,
        .addr = 0b00000000,
        .length = 0,
        .rxlength = 0,
        .user = NULL,
        .tx_buffer = NULL,
        .rx_buffer = NULL,
    };
    spi_device_polling_transmit(spi_device_LCD, &spi_envio_LCD_CS);
}