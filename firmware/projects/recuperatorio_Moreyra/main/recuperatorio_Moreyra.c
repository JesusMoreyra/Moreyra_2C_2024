/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	ECHO	 	| 	GPIO_3	|
 * | 	TRIGGER	 	| 	GPIO_2	|
 * | 3.3V	 	| 	3.3V		|
 * | GND	 	| 	GND			|
 * | Galga 1	| 	CH0			|
 * | Galga 2 	| 	CH1			|
 *
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 11/11/2024 | Document creation		                         |
 *
 * @author Jesus Moreyra (jesusbenja25@gmail.com)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "timer_mcu.h"
#include "gpio_mcu.h"
#include "led.h"
#include "hc_sr04.h"
/*==================[macros and definitions]=================================*/
#define barrera GPIO_6
#define CONFIG_PERIOD_US_VELOCIDAD 100000 // 10 veces por segundo
#define CONFIG_PERIOD_US_PESADO 5000	  // 200 veces por segundo
/*==================[internal data definition]===============================*/
uint16_t medicion = 0;
uint16_t distanciaActual = 0;
uint16_t distanciaAnterior = 0;
uint16_t velocidad = 0;
uint16_t velocidadMaxima = 0;
uint16_t distanciaCamion;
bool camionParado = false;
uint16_t galga1 = 0;
uint16_t galga2 = 0;
uint16_t sumaGalga1 = 0;
uint16_t sumaGalga2 = 0;
uint16_t pesoCamion = 0;
TaskHandle_t velocidadCamion_handle = NULL;
TaskHandle_t pesadoCamion_handle = NULL;
/*==================[internal functions declaration]=========================*/
/**
 * @brief Calcula la velocidad a la que viene el camion
 *
 * a traves de mediciones obtenidas por el sensor HC-SR04 se encarga de calcular la velocidad del camion
 * y de encender los leds de acuerdo a la velocidad
 *
 * @param pvParameter No se utiliza
 */
void calculoVelocidadCamion(void *pvParameter)
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		distanciaCamion = HcSr04ReadDistanceInCentimeters();

		if (distanciaCamion < 1000) // menor a 10 metros
		{
			medicion = HcSr04ReadDistanceInCentimeters();
			distanciaActual = medicion / 100;						 // se divide por 100 para obtener la distancia en metros
			velocidad = (distanciaAnterior - distanciaActual) / 0.1; // se divide por 0.1 para obtener la velocidad en metros por segundo
			// la dirección de avance de los vehículos es siempre hacia el sensor
			// Es siempre hacia el sensor entonces es anterior - actual
			distanciaAnterior = distanciaActual;

			if (velocidad > 8)
				LedOn(LED_3);
			else
				LedOff(LED_3);

			if (velocidad > 0 && velocidad < 8)
				LedOn(LED_2);
			else
				LedOff(LED_2);

			if (velocidad == 0)
			{
				LedOn(LED_1);
				camionParado = true;
			}
			else
				LedOff(LED_1);

			if (velocidad > velocidadMaxima)
				velocidad = velocidadMaxima;
		}
	}
}


/**
 * @brief Pesado del camion
 *
 * La tarea pesadoCamion se encarga de pesar el camion
 * cuando este se detiene.
 *
 * @param pvParameter No se utiliza
 */
void pesadoCamion(void *pvParameter)
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (camionParado)
		{
			for (int i = 0; i < 50; i++)
			{
				AnalogInputReadSingle(CH0, &galga1);

				galga1 = (galga1 * 20000) / 3300; // convierto mv a kg
				sumaGalga1 += galga1;
			}

			for (int i = 0; i < 50; i++)
			{
				AnalogInputReadSingle(CH1, &galga2);
				galga2 = (galga2 * 20000) / 3300; // convierto mv a kg
				sumaGalga2 += galga2;
			}
			pesoCamion = (sumaGalga1 / 50) + (sumaGalga2 / 50);

			UartSendString(UART_PC, "Peso: ");
			UartSendString(UART_PC, (char *)UartItoa(pesoCamion, 10));
			UartSendString(UART_PC, "kg");
			UartSendString(UART_PC, "\r\n");

			UartSendString(UART_PC, "Velocidad maxima: ");
			UartSendString(UART_PC, (char *)UartItoa(velocidadMaxima, 10));
			UartSendString(UART_PC, "m/s");
			UartSendString(UART_PC, "\r\n");
		}
	}
}

/**
 * @brief
 *  Interrupcion generada por el timer que informa cuando
 *  se debe calcular la velocidad del camion.
 *
 * @param[in] param No se utiliza
 */
void FuncTimerVelocidad(void *param)
{
	vTaskNotifyGiveFromISR(velocidadCamion_handle, pdFALSE);
}

/**
 * @brief Interrupción generada por el timer que informa cuando
 * se debe calcular el peso del camión.
 *
 * Esta función se invoca en la interrupción del timer de pesado y 
 * envía una notificación a la tarea asociada para procesar el cálculo.
 *
 * @param[in] param Puntero a un parámetro no utilizado.
 */
void FuncTimerPesado(void *param)
{
	vTaskNotifyGiveFromISR(pesadoCamion_handle, pdFALSE);
}

/**
 * @brief Tarea que atiende la recepcion de datos en el puerto serial.
 *
 * Esta tarea se encarga de leer los caracteres que se envian
 * por el puerto serial y de encender o apagar la barrera
 * segun sea el caso.
 *
 * @param[in] pvParameter No se utiliza
 */
void FuncUart(void *pvParameter)
{
	uint8_t caracter;
	UartReadByte(UART_PC, &caracter);
	switch (caracter)
	{
	case 'o':
		GPIOOn(barrera);
		break;
	case 'c':
		GPIOOff(barrera);
		break;
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{

	HcSr04Init(GPIO_3, GPIO_2);
	LedsInit();

	analog_input_config_t galga1 = {

		.input = CH0,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = NULL,
	};

	AnalogInputInit(&galga1);

	analog_input_config_t galga2 = {

		.input = CH1,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = NULL,
	};
	AnalogInputInit(&galga2);

	serial_config_t my_uart = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = FuncUart,
		.param_p = NULL};

	UartInit(&my_uart);

	timer_config_t timerVelocidad = {
		.timer = TIMER_A,
		.period = CONFIG_PERIOD_US_VELOCIDAD,
		.func_p = FuncTimerVelocidad,
		.param_p = NULL};
	TimerInit(&timerVelocidad);
	TimerStart(timerVelocidad.timer);

	timer_config_t timerPesado = {
		.timer = TIMER_B,
		.period = CONFIG_PERIOD_US_PESADO,
		.func_p = FuncTimerPesado,
		.param_p = NULL};
	TimerInit(&timerPesado);
	TimerStart(timerPesado.timer);

	xTaskCreate(&calculoVelocidadCamion, "Velocidad", 512, NULL, 5, &velocidadCamion_handle);
	xTaskCreate(&pesadoCamion, "Pesado", 2048, NULL, 5, &pesadoCamion_handle);
}
/*==================[end of file]============================================*/

















