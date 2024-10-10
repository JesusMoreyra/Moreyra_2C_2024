/*! @mainpage guia3_ej4.c
 *
 * @section genDesc General Description
 *
 * Se implementa una aplicación de adquisición y transmisión de señal analógica basada en los drivers analog_io_mcu.h 
 * y uart_mcu.h. El objetivo es capturar una señal analógica a través del canal CH1 del convertidor 
 * AD (Analógico a Digital), digitalizarla, y luego enviarla a un graficador en la PC a través de la UART.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    ESP 32		|  Periferico 	|
 * |:--------------:|:--------------|
 * | 	Ch1	 	| 	Pata derecha del potenciómetro		|
 * | 	Ch0	 	| 	Pata central del potenciómetro		|
 * | 	GND	 	| 	Pata izquierda del potenciómetro	|
 * 
 * 
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 10/10/2024 | Document creation		                         |
 *
 * @author Jesus Moreyra (jesusbenja25@gmail.com)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "analog_io_mcu.h"
#include "freertos/FreeRTOS.h"
#include "uart_mcu.h"
#include "freertos/task.h"
#include "timer_mcu.h"


/*==================[macros and definitions]=================================*/
#define CONFIG_PERIOD_US 2 * 1000 
#define CONFIG_PERIOD_US_write 4 * 1000 
#define BUFFER_SIZE 231

/*==================[internal data definition]===============================*/
TaskHandle_t Input_task_handle = NULL;
TaskHandle_t Output_task_handle = NULL;
const char ecg[BUFFER_SIZE] = {
	76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76,
};

/*==================[internal functions declaration]=========================*/
/**
 * @brief Envía una notificación a la tarea de lectura.
 *
 * Esta función es invocada en la interrupción del timer de lectura y envía una notificación a la tarea asociada.
 *
 * @param param Puntero a un parámetro no utilizado.
 */
void FuncTimerRead(void *param)
{
	vTaskNotifyGiveFromISR(Input_task_handle, pdFALSE); /* Envía una notificación a la tarea asociada */
}
/**
 * @brief Envía una notificación a la tarea de escritura de la salida analógica.
 *
 * Esta función es invocada en la interrupción del timer de escritura de la salida analógica y envía una notificación a la tarea asociada.
 *
 * @param param Puntero a un parámetro no utilizado.
 */
void FuncTimerWrite(void *param)
{
	vTaskNotifyGiveFromISR(Output_task_handle, pdFALSE); /* Envía una notificación a la tarea asociada */
}
/**
 * @brief Lee un valor de entrada analógica y lo envía por puerto serie.
 * 
 * Esta tarea se ejecuta indefinidamente y espera notificaciones. Cuando se notifica, lee el valor de la entrada analógica configurada y lo envía por puerto serie.
 * 
 * @param pvParameter Puntero a un parámetro no utilizado.
 * 
 * @return Ninguno
 */
static void InputReadSingle(void *pvParameter)
{
	uint16_t valorLectura = 0;
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		AnalogInputReadSingle(CH1, &valorLectura);
		UartSendString(UART_PC, (char *)UartItoa(valorLectura, 10));
		UartSendString(UART_PC, "\r\n");
	}
}
/**
 * @brief Tarea que escribe en la salida analógica el valor guardado en el array ecg.
 * 
 * Esta tarea se ejecuta indefinidamente y espera notificaciones. Cuando se notifica, escribe el valor en el buffer de salida correspondiente al índice actual y 
 * incrementa el índice en 1. Si el índice supera el tamaño del buffer, se reinicia a 0.
 * 
 * @param pvParameter Un puntero void que no se utiliza dentro de la función.
 * 
 * @return Ninguno
 */
static void OutputWrite(void *pvParameter)
{
	uint8_t ecgIndex = 0;
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (ecgIndex == BUFFER_SIZE)
			ecgIndex = 0;
		AnalogOutputWrite(ecg[ecgIndex]);
		ecgIndex++;
		if (ecgIndex == BUFFER_SIZE)
			ecgIndex = 0;
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{

	analog_input_config_t config;

	config.input = CH1;
	config.mode = ADC_SINGLE;
	config.func_p = NULL;
	config.param_p = NULL;
	config.sample_frec = NULL;

	AnalogInputInit(&config);
	AnalogOutputInit();
	/* Timer configuration */
	timer_config_t Input_Read = {
		.timer = TIMER_A,
		.period = CONFIG_PERIOD_US,
		.func_p = FuncTimerRead,
		.param_p = NULL};
	TimerInit(&Input_Read);

	timer_config_t Output_Write = {
		.timer = TIMER_B,
		.period = CONFIG_PERIOD_US_write,
		.func_p = FuncTimerWrite,
		.param_p = NULL};
	TimerInit(&Output_Write);

	serial_config_t my_uart = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = NULL,
		.param_p = NULL};
	UartInit(&my_uart);

	xTaskCreate(&InputReadSingle, "InputReadSingle", 1024, NULL, 1, &Input_task_handle);
	xTaskCreate(&OutputWrite, "OutputWrite", 1024, NULL, 1, &Output_task_handle);
	TimerStart(Input_Read.timer);
	TimerStart(Output_Write.timer);
}
/*==================[end of file]============================================*/