/*
Funciones para controlar el bus I2C
https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32c3/api-reference/peripherals/i2c.html
*/
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "i2c_bus.h"
#include "pinOut.h"

#define STACK_SIZE 8048 //Stack de las tareas.
//SHT40
#define BYTES_TRANSMISION 1
#define BYTES_RECEPCION 6
#define COMANDO_SHT40_LEER 0xFD
//MAX17048
#define MAX17040_VCELL	0x02
#define MAX17040_SOC	0x04
//RTC MCP7940N
//#define FIJAR_HORA_RTC //Una vez grabado deshabilitar y volver a programar para que el RTC conserve la hora.

//Reserva en RAM del stack and task control block (TCB) para las tareas creadas estáticas
static StackType_t stack_tarea_i2c[STACK_SIZE];
static StaticTask_t TCB_tarea_i2c;
//Declaramos los handlers para las tasks estáticas por si queremos suspenderlas desde otras tasks.
TaskHandle_t xHandle_i2c_crear_tarea_i2c = NULL;

//Función asignada a la tarea i2c
static void tarea_i2c(void *pvParameters);

/************** Definición de las funciones ****************************/

//Función para crear la tarea con la que se pruena el bus i2c
void i2c_crear_tarea_i2c(void)
{
    xHandle_i2c_crear_tarea_i2c = xTaskCreateStatic(
        &tarea_i2c,              //Función que se asigna a al tarea.
        "tarea_i2c",           //Nombre que le asignamos a la tarea (debug). 
        STACK_SIZE,            //Tamaño del stack de la tarea (reservado de manera estática previamente).
        NULL,                  //Parámetros pasados a la tarea en su creación.
        tskIDLE_PRIORITY + 1,  //Prioridad de la tarea. 
        stack_tarea_i2c,       //Array para el stack de la tarea reservado previamente. 
        &TCB_tarea_i2c);       //Memoria reservada previamente para el TCB (task control block) de la tarea.
    
        printf("Tarea para probar el bus I2C creada.\n");  
}

static void tarea_i2c(void *pvParameters)
{
    //Configuramos el ESP como maestro.
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = 0,
        .scl_io_num = I2C_SCL,
        .sda_io_num = I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    //Añadimos el SHT40 al bus I2C
    i2c_device_config_t sht40_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x45,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t sht40_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &sht40_cfg, &sht40_handle));

    //Añadimos el MAX17048 al bus I2C
    i2c_device_config_t max17048_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x36,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t max17048_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &max17048_cfg, &max17048_handle));

    //Añadimos el MCP7940N al bus I2C
    i2c_device_config_t mcp7940n_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x6F,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t mcp7940n_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &mcp7940n_cfg, &mcp7940n_handle));

    //Variables para el SHT40
    uint8_t data_wr[1];
    uint8_t data_rd[6];
    float temperatura = 0;
    float humedad = 0;
    int temperatura_recibida = 0;
    int humedad_recibida = 0;

    //Variables para el MAX17048
    uint8_t max_data_wr[1];
    uint8_t max_data_rd[2];
    float voltaje = 0;
    uint8_t soc = 0;

    //Variables para el MCP7940N
    uint8_t mcp_data_wr[2] = {0, 0};
    uint8_t mcp_data_rd[3] = {0, 0, 0};
    uint8_t segundos_bcd = 0;
    uint8_t minutos_bcd = 0;
    uint8_t horas_bcd = 0;
    uint8_t segundos = 0;
    uint8_t minutos = 0;
    uint8_t horas = 0;
    uint8_t diaSemana = 0;
    uint8_t diaMes = 0;
    uint8_t mes = 0;
    uint8_t year = 0;

    while(1)
    {
        //Leemos el sensor SHT40 e imprimimos sus lecturas.
        data_wr[0] = COMANDO_SHT40_LEER; 
        ESP_ERROR_CHECK(i2c_master_transmit(sht40_handle, data_wr, BYTES_TRANSMISION, -1));
        vTaskDelay(10 / portTICK_PERIOD_MS);
        i2c_master_receive(sht40_handle, data_rd, BYTES_RECEPCION, -1);
        //Calculamos los valores de temperatura y humedad recibidos.
        temperatura_recibida = (data_rd[0]*256L) + data_rd[1];
        temperatura = -45 + 175*(temperatura_recibida/65535.0f);
        humedad_recibida = (data_rd[3]*256L) + data_rd[4];
        humedad = -6 + 125*(humedad_recibida/65535.0f);
        //Imprimimos los valores de temperatura y humedad
        printf("Lectura SHT40: Temperatura %.1f.\n", temperatura);
        printf("Lectura SHT40: Humedad %.1f.\n", humedad);

        //Leemos el MAX17048 e imprimimos sus lecturas.
        max_data_wr[0] = MAX17040_SOC; //Registro con el valor del SOC.
        ESP_ERROR_CHECK(i2c_master_transmit_receive(max17048_handle, max_data_wr, sizeof(max_data_wr), max_data_rd, 2, -1));
        //Calculamos el valor del SOC.
        soc = max_data_rd[0];
        printf("Lectura MAX17040: Porcentaje de bateria %u %%.\n", soc);
        max_data_wr[0] = MAX17040_VCELL; //Registro con el valor de Vcell
        ESP_ERROR_CHECK(i2c_master_transmit_receive(max17048_handle, max_data_wr, sizeof(max_data_wr), max_data_rd, 2, -1));
        //Calculamos el valor de Vcell.
        voltaje = ((max_data_rd[0]*256L + max_data_rd[1])*78.125f)/1000000;
        printf("Lectura MAX17040: Voltaje de bateria %.3f V.\n", voltaje);
        //ESP_ERROR_CHECK(i2c_master_transmit_receive(max17048_handle, max_data_wr, sizeof(max_data_wr), max_data_rd, 2, -1));

        //Leemos el MCP7940N e imprimimos sus lecturas.
        #ifdef FIJAR_HORA_RTC
        //Configuramos la hora en el RTC, volver a programar deshabilitando esta opción una vez hecho.
        year = 0b00100101; //Valores en BCD.
        mes = 0b00000101;
        diaMes = 0b00000110;
        diaSemana = 0b00001010; //Habilitamos Vbaten
        horas = 0b00011001;
        minutos = 0b00010110;
        //Deshabilitamos el oscilador
        mcp_data_wr[0] = 0x00; //Escribimos en el registro 0x00
        mcp_data_wr[1] = 0b00000000; 
        ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
        //Escribimos los minutos
        mcp_data_wr[0] = 0x01; //Escribimos en el registro 0x01
        mcp_data_wr[1] = minutos; 
        ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
        //Escribimos las horas
        mcp_data_wr[0] = 0x02; //Escribimos en el registro 0x02
        mcp_data_wr[1] = horas; 
        ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
        //Escribimos el día de la semana y habilitamos la batería del RTC.
        mcp_data_wr[0] = 0x03; //Escribimos en el registro 0x03
        mcp_data_wr[1] = diaSemana; 
        ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
        //Escribimos el día del mes.
        mcp_data_wr[0] = 0x04; //Escribimos en el registro 0x04
        mcp_data_wr[1] = diaMes; 
        ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
        //Escribimos el mes
        mcp_data_wr[0] = 0x05; //Escribimos en el registro 0x05
        mcp_data_wr[1] = mes; 
        ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
        //Escribimos el años
        mcp_data_wr[0] = 0x06; //Escribimos en el registro 0x06
        mcp_data_wr[1] = year; 
        ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
        //Escribimos los segundos y habilitamos el oscilador
        //Deshabilitamos el oscilador
        mcp_data_wr[0] = 0x00; //Escribimos en el registro 0x00
        mcp_data_wr[1] = 0b10000000; 
        ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
        #endif

        //Escribimos en el registro 0x00 para posteriormente leer desde esa dirección.
        mcp_data_wr[0] = 0x00;
        ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 1, -1));
        i2c_master_receive(mcp7940n_handle, mcp_data_rd, 3, -1);

        segundos_bcd = mcp_data_rd[0];
        minutos_bcd = mcp_data_rd[1];
        horas_bcd = mcp_data_rd[2];
        //Convertimos de BCD a decimal
        segundos = (((segundos_bcd >> 4) * 10) + (segundos_bcd & 0x0F)) - 80; //Leemos el bit del cristal habilitado
        minutos = ((minutos_bcd >> 4) * 10) + (minutos_bcd & 0x0F);
        horas = ((horas_bcd >> 4) * 10) + (horas_bcd & 0x0F);

        printf("Lectura MCP7940N: Hora %02u:%02u:%02u.\n\n", horas, minutos, segundos);        

        //Bloqueamos la tarea durante 300s
        vTaskDelay(300000 / portTICK_PERIOD_MS);   
    }
}
