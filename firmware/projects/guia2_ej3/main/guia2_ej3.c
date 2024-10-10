/*! @mainpage guia2_ej3
 *
 * @section genDesc General Description
 *
 * En este proyecto se modifica el proyecto anterior (que usa interrupciones y Timers) para agregar un puerto serie 
 * que permita enviar los datos de las mediciones al Monitor Serial. Este proyecto incorpora la comunicación serie, 
 * lo que permite observar las distancias medidas en tiempo real y manejar el sistema desde el teclado del ordenador.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |   ESP32    | Periferico|
 *	|:----------:|:-----------|
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
 * | 10/10/2024| Document creation		                         |
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
#include "timer_mcu.h"
#include "lcditse0803.h"
#include "uart_mcu.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_PERIOD_US 1000 * 1000
/*==================[internal data definition]===============================*/
uint16_t distance = 0;
bool toggle = false;
bool hold = false;
TaskHandle_t leds_task_handle = NULL;
TaskHandle_t measure_task_handle = NULL;
TaskHandle_t lcd_task_handle = NULL;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Función invocada en la interrupción del timer measure
 */
void FuncTimerMeasure(void *param)
{
	vTaskNotifyGiveFromISR(measure_task_handle, pdFALSE); /* Envía una notificación a la tarea asociada */
}
/**
 * @brief Función invocada en la interrupción del timer LCD
 */
void FuncTimerLCD(void *param)
{
	vTaskNotifyGiveFromISR(lcd_task_handle, pdFALSE); /* Envía una notificación a la tarea asociada  */

	vTaskNotifyGiveFromISR(leds_task_handle, pdFALSE);
}
/**
 * @brief Función invocada en la interrupción del timer leds
 */
void FuncTimerLeds(void *param)
{
	vTaskNotifyGiveFromISR(leds_task_handle, pdFALSE); /* Envía una notificación a la tarea asociada */
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
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (toggle)
			distance = HcSr04ReadDistanceInCentimeters();
	}
}
/**
 * @brief Función de tarea que controla el estado de los LEDs según el valor de la distancia actual.
 * 
 * Esta función se ejecuta indefinidamente y espera notificaciones. Cuando se notifica, verifica el valor de la distancia y establece los LEDs en consecuencia.
 * 
 * @param pvParameter Un puntero void que no se utiliza dentro de la función.
 * @return Ninguno
 */
static void LedsTask(void *pvParameter)
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		if (toggle)
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
		}
		else
			LedsOffAll();
	}
}
/**
 * @brief Tarea responsable de actualizar la pantalla LCD.
 * 
 * Esta función se ejecuta indefinidamente y verifica la bandera toggle para determinar si mostrar la distancia actual o apagar la pantalla LCD.
 * 
 * @param pvParameter Puntero al parámetro de la tarea, no utilizado en esta función.
 * 
 * @return Ninguno
 */
static void LCDTask(void *pvParameter)
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if (toggle)
		{
			UartSendString(UART_PC, (char *)UartItoa(distance, 10));
			UartSendString(UART_PC, " cm\r\n");
			if (!hold)
			{
				LcdItsE0803Write(distance);
			}
		}
		else
			LcdItsE0803Off();
	}
}
/**
 * @brief Alterna el estado de la variable toggle.
 *
 * Esta función invierte el estado actual de la variable toggle.
 *
 * @param Ninguno
 * @return Ninguno
 */
void Key1(void)
{
	toggle = !toggle;
}
/**
 * @brief Invierte el estado de la variable hold.
 *
 * Esta función invierte el estado actual de la variable hold.
 *
 * @param None
 * @return None
 */
void Key2(void)
{
	hold = !hold;
}
/**
	 * @brief Función de callback de UART.
	 * 
	 * Esta función es llamada cuando se recibe datos a través de la interfaz UART.
	 * Lee un solo byte del puerto UART y realiza acciones según el carácter recibido.
	 * 
	 * @param pvParameter Puntero al parámetro de la función de callback (no utilizado en esta implementación).
	 * 
	 * @return Ninguno
	 */
void FuncUart(void *pvParameter)
{
	uint8_t caracter;
	UartReadByte(UART_PC, &caracter);
	switch (caracter)
	{
	case 'O':
		Key1();
		break;
	case 'H':
		Key2();
		break;
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	LedsInit();
	SwitchesInit();
	HcSr04Init(GPIO_3, GPIO_2);
	LcdItsE0803Init();
	/* Timer configuration */
	timer_config_t timer_measure = {
		.timer = TIMER_A,
		.period = CONFIG_PERIOD_US,
		.func_p = FuncTimerMeasure,
		.param_p = NULL};
	TimerInit(&timer_measure);

	timer_config_t timer_lcd = {
		.timer = TIMER_B,
		.period = CONFIG_PERIOD_US,
		.func_p = FuncTimerLCD,
		.param_p = NULL};
	TimerInit(&timer_lcd);
	TimerStart(timer_lcd.timer);
	TimerStart(timer_measure.timer);

	serial_config_t my_uart = {
		.port = UART_PC,
		.baud_rate = 9600,
		.func_p = FuncUart,
		.param_p = NULL};
	UartInit(&my_uart);

	SwitchActivInt(SWITCH_1, Key1, NULL);
	SwitchActivInt(SWITCH_2, Key2, NULL);
	xTaskCreate(&LedsTask, "LED_2", 512, NULL, 5, &leds_task_handle);
	xTaskCreate(&MeasureTask, "LED_2", 512, NULL, 5, &measure_task_handle);
	xTaskCreate(&LCDTask, "LED_3", 512, NULL, 5, &lcd_task_handle);
}
/*==================[end of file]============================================*/