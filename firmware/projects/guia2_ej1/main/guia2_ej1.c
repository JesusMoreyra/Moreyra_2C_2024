/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * En este ejercicio se implementa una aplicación de un medidor de distancia basado en un sensor de ultrasonido HC-SR04
 *  y una pantalla LCD utilizando el ESP32. El objetivo principal es diseñar y 
 * programar el firmware para controlar el sensor, interpretar las distancias medidas, y mostrar los resultados tanto en
 *  un display LCD como mediante LEDs
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |   ESP32   | Periferico|
 * |:----------:|:-----------|
 * | GPIO_20    | 		D1   |
 * | GPIO_21    | 		D2 |
 * | GPIO_22    | 		D3   |
 * | GPIO_23    | 		D4   |
 * | GPIO_19    | 		SEL_1 |
 * | GPIO_18    | 		SEL_2   |
 * | GPIO_9    | 		SEL_3   |
 * | +5V    | 		+5V   |
 * | GND    | 		GND   |
 * | GPIO_3    | 	ECHO	|
 * | GPIO_2    | 	TRIGGER |
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Moreyra Jesus (jesusbenja25@gmail.com)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
#include "hc_sr04.h"
#include "gpio_mcu.h"
#include "lcditse0803.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD 1000
#define CONFIG_BLINK_PERIOD_3 250
#define CONFIG_BLINK_PERIOD_2 100 

/*==================[internal data definition]===============================*/
uint16_t distance = 0;
bool toggle = false;
bool hold = false;
TaskHandle_t key_task_handle = NULL;
TaskHandle_t leds_task_handle = NULL;
TaskHandle_t measure_task_handle = NULL;
TaskHandle_t lcd_task_handle = NULL;
/*==================[internal functions declaration]=========================*/

/**
 * @brief Tarea responsable de leer teclas.
 * 
 * Esta función se ejecuta indefinidamente y lee el estado de las teclas. Si se detecta una tecla presionada, se actualiza la bandera toggle o hold.
 * 
 * @param pvParameter Un puntero al parámetro de la tarea, no utilizado en esta función.
 * 
 * @return Ninguno
 */
static void KeyTask(void *pvParameter)
{
	while (true)
	{
		uint8_t teclas;
		teclas = SwitchesRead();
		if (teclas == SWITCH_1)
			toggle = !toggle;
		if (teclas == SWITCH_2)
		{
			hold = !hold;
		}
		vTaskDelay(CONFIG_BLINK_PERIOD_2 / portTICK_PERIOD_MS); // usar otro delay
	}
}

/**
 * @brief Tarea responsable de medir la distancia mediante el sensor HcSr04.
 * 
 * Esta función se ejecuta indefinidamente y verifica la bandera toggle para determinar si realizar la medición de la distancia.
 * 
 * @param pvParameter Un puntero al parámetro de la tarea, no utilizado en esta función.
 * 
 * @return Ninguno
 */
static void MeasureTask(void *pvParameter)
{
	while (true)
	{
		if (toggle)
			distance = HcSr04ReadDistanceInCentimeters();
		vTaskDelay(CONFIG_BLINK_PERIOD_3 / portTICK_PERIOD_MS);
	}
}
/**
 * La función LedsTask controla el estado de los LEDs en función del valor actual de la distancia.
 * 
 * Esta función se ejecuta de manera indefinida y verifica el valor de la distancia para determinar qué LEDs encender o apagar.
 * Los LEDs se controlan de la siguiente manera:
 * - Si la distancia es menor o igual a 10, todos los LEDs se apagan.
 * - Si la distancia está entre 10 y 20, solo el LED_1 se enciende.
 * - Si la distancia está entre 20 y 30, los LEDs LED_1 y LED_2 se encienden.
 * - Si la distancia es mayor o igual a 30, todos los LEDs se encienden.
 * 
 * La función toma un puntero a void como parámetro, pero no se utiliza dentro de la función.
 * La función no devuelve ningún valor.
 * 
 * @param pvParameter Un puntero a void que no se utiliza dentro de la función.
 * @return Ninguno
 */
static void LedsTask(void *pvParameter)
{
	while (true)
	{
		if (distance <= 10)
		{
			LedOff(LED_1);
			LedOff(LED_2);
			LedOff(LED_3);
		}
		if (distance >= 10 && distance <= 20)
		{
			LedOn(LED_1);
			LedOff(LED_2);
			LedOff(LED_3);
		}
		
		if (distance >= 20 && distance <= 30)
		{
			LedOn(LED_1);
			LedOn(LED_2);
			LedOff(LED_3);
		}
		if (distance >= 30)
		{
			LedOn(LED_1);
			LedOn(LED_2);
			LedOn(LED_3);
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
/**
 * @brief Tarea responsable de actualizar la pantalla LCD.
 * 
 * Esta función se ejecuta indefinidamente y verifica la bandera toggle para determinar si mostrar la distancia actual o apagar la pantalla LCD.
 * 
 * @param pvParameter Un puntero al parámetro de la tarea, no utilizado en esta función.
 * 
 * @return Ninguno
 */
static void LCDTask(void *pvParameter)
{
	while (true)
	{
		if (toggle)
		{
			if (!hold)
			{
				LcdItsE0803Write(distance);
			}
		}
		else
			LcdItsE0803Off();
		vTaskDelay(CONFIG_BLINK_PERIOD_2 / portTICK_PERIOD_MS);
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	LedsInit();
	SwitchesInit();
	HcSr04Init(GPIO_3, GPIO_2);
	LcdItsE0803Init();
	xTaskCreate(&KeyTask, "Tarea_Teclas", 512, NULL, 5, &key_task_handle);
	xTaskCreate(&LedsTask, "LED_2", 512, NULL, 5, &leds_task_handle);
	xTaskCreate(&MeasureTask, "LED_3", 512, NULL, 5, &measure_task_handle);
	xTaskCreate(&LCDTask, "LED_3", 512, NULL, 5, &lcd_task_handle);
}
/*==================[end of file]============================================*/