/*
Programa para verificar el funcionamiento del hardware.
Escribe en el LCD por SPI.
Lee lo integrados del bus I2C. 
Enciende y apaga los leds, y suena el pulsador al pulsar un botón.
*/
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_log.h"

#include "pinOut.h"
#include "gpios.h"
#include "lcd.h"
#include "i2c_bus.h"

void app_main(void)
{
    gpios_crear_tarea();
    lcd_crear_tarea_LCD();
    i2c_crear_tarea_i2c();
}