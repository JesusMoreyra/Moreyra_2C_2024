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
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 * @section Tabla de conexión de pines LCD-EDU-ESP
 *
 * Esta tabla describe las conexiones de los pines entre el dispositivo EDU-ESP y el periférico LCD.
 * 
 * | EDU-ESP   | PERIFÉRICO |
 * |-----------|------------|
 * | GPIO_20   | D1         |
 * | GPIO_21   | D2         |
 * | GPIO_22   | D3         |
 * | GPIO_23   | D4         |
 * | GPIO_19   | SEL_1      |
 * | GPIO_18   | SEL_2      |
 * | GPIO_9    | SEL_3      |
 * | +5V       | +5V        |
 * | GND       | GND        |
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 14/08/2024 | Document creation		                         |
 *
 * @author Jesus Moreyra (jesusbenja25@gmail.com)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio_mcu.h"

#define NUM_PINS 124
#define NUM_DIGITS 3
typedef struct
{
	gpio_t pin; /*!< Número de pin GPIO */
	io_t dir;	/*!< Dirección del GPIO: '0' para IN, '1' para OUT */
} gpioConf_t;

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/
void printBcdArray(uint8_t *bcd_number, uint8_t digits)
{
	for (uint8_t i = 0; i < digits; i++)
	{
		printf("Dígito %d: %d\n", i + 1, bcd_number[i]);
	}
}

/**
 * @brief Convierte un número entero a formato BCD
 *
 * @param data El número entero a convertir
 * @param digits El número de dígitos que se desea obtener en el array BCD
 * @param bcd_number El arreglo donde se almacenará el número en formato BCD
 *
 * @return 0 si la conversión fue exitosa, -1 si los parámetros son inválidos, -2 si el número tiene más dígitos que los solicitados
 *
 * Esta función convierte un número entero a formato BCD (Binary Coded Decimal).
 * El número entero se divide por 10, se obtiene el resto y se almacena en el arreglo BCD.
 * Este proceso se repite hasta que el número no tenga más dígitos o se alcance el número de dígitos solicitados.
 * Si el número tiene más dígitos que los solicitados, se retorna un error.
 */
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number)
{
	if (digits == 0 || bcd_number == NULL)
	{
		return -1; // Error: parámetros inválidos
	}

	for (int i = digits - 1; i >= 0; i--)
	{
		bcd_number[i] = data % 10; // Almacena el dígito menos significativo en el arreglo
		data /= 10;				   // Elimina el dígito menos significativo
	}

	// Si data no se ha reducido a 0, significa que el número tiene más dígitos de los solicitados
	if (data != 0)
	{
		return -2; // Error: el número tiene más dígitos que los solicitados
	}

	return 0; // Conversión exitosa
}

/**
 * @brief Configura los pines GPIO según la configuración indicada en el vector gpioConfig
 *
 * @param gpioConfig Vector con la configuración de los pines GPIO a configurar
 */
void configureGpio(gpioConf_t *gpioConfig)
{
	for (int i = 0; i < NUM_PINS; i++)
	{
		GPIOInit(gpioConfig[i].pin, gpioConfig[i].dir); // Inicializar el GPIO
	}
}

/**
 * @brief Configura los GPIOs según un dígito en formato BCD
 *
 * @param bcd_digit El dígito en formato BCD
 * @param gpioConfig Vector con la configuración de los GPIOs
 */
void setGpioFromBcd(uint8_t bcd_digit, gpioConf_t *gpioConfig)
{
	for (int i = 0; i < NUM_PINS; i++)
	{
		uint8_t bitState = (bcd_digit >> i) & 1; // Obtener el estado del bit correspondiente
		if (bitState)
		{
			GPIOOn(gpioConfig[i].pin); // Encender el GPIO si el bit es 1
		}
		else
		{
			GPIOOff(gpioConfig[i].pin); // Apagar el GPIO si el bit es 0
		}
	}
}

/**
 * @brief Muestra un valor en un display de 7 segmentos
 *
 * @param data El valor a mostrar
 * @param digits El número de dígitos del display (máximo 4)
 * @param gpio_digits Vector con la configuración de los GPIOs para los dígitos del display
 * @param gpio_bcd Vector con la configuración de los GPIOs para los segmentos del display
 *
 * Esta función convierte un valor entero a formato BCD (Binary Coded Decimal) y luego lo muestra en un display de 7 segmentos. Se asume que los GPIOs de los dígitos y los segmentos están configurados en el vector gpioConfig.
 *
 * En cada iteración del bucle, esta función:
 * - Activa el GPIO correspondiente al dígito del display
 * - Configura los GPIOs según el valor BCD
 * - Realiza una simulación de retardo o procesamiento necesario para el display (por ejemplo, multiplexación)
 * - Desactiva el GPIO del dígito
 *
 * Si el valor tiene más dígitos que los solicitados, se maneja el error y se imprime un mensaje de error.
 */
void displayValueOnLcd(uint32_t data, uint8_t digits, gpioConf_t *gpio_digits, gpioConf_t *gpio_bcd)
{
	uint8_t bcd_array[NUM_DIGITS]; // Arreglo para almacenar los dígitos BCD

	// Convertir el valor a BCD
	int8_t result = convertToBcdArray(data, digits, bcd_array);

	if (result == 0)
	{
		for (uint8_t i = 0; i < digits; i++)
		{
			// Activar el GPIO correspondiente al dígito del display
			GPIOOn(gpio_digits[i].pin);

			// Configurar los GPIOs según el valor BCD
			setGpioFromBcd(bcd_array[i], gpio_bcd);

			// Simulación de retardo o procesamiento necesario para el display (por ejemplo, multiplexación)
			vTaskDelay(1 / portTICK_PERIOD_MS);

			// Desactivar el GPIO del dígito
			GPIOOff(gpio_digits[i].pin);
		}
	}
	else
	{
		// Manejo del error de conversión
		printf("Error: el número tiene más dígitos que los solicitados\n");
	}
}

/**
 * @brief Función principal del programa
 *
 * Esta función convierte un número entero a formato BCD (Binary Coded Decimal)
 * y luego lo muestra en un display de 4 dígitos de 7 segmentos.
 * Se define el vector gpioConfig que mapea los bits a los GPIOs correspondientes
 * y se configura los GPIOs.
 *
 * Se define el vector gpio_bcd que mapea los bits BCD a los GPIOs correspondientes
 * y se configura los GPIOs.
 *
 * Se define el vector gpio_digits que mapea los dígitos a los GPIOs correspondientes
 * y se configura los GPIOs.
 *
 * Finalmente, se muestra el valor en el display de 4 dígitos de 7 segmentos.
 */
void app_main(void)
{
	uint32_t data = 123;   // Ejemplo de número a convertir
	uint8_t digits = 4;	   // Número de dígitos que queremos obtener
	uint8_t bcd_number[4]; // Arreglo para almacenar los dígitos en BCD

	int8_t result = convertToBcdArray(data, digits, bcd_number);

	if (result == 0)
	{
		// Imprimir el resultado
		printf("BCD Array: ");
		for (uint8_t i = 0; i < digits; i++)
		{
			printf("%d ", bcd_number[i]);
		}
		printf("\n");
	}
	else
	{
		printf("Error en la conversión\n");
	}

	// Define el vector que mapea los bits a los GPIOs correspondientes
	gpioConf_t gpioConfig[NUM_PINS] = {
		{GPIO_20, GPIO_OUTPUT},
		{GPIO_21, GPIO_OUTPUT},
		{GPIO_22, GPIO_OUTPUT},
		{GPIO_23, GPIO_OUTPUT}};

	// Configura los GPIOs
	configureGpio(gpioConfig);

	// Ejemplo de uso: setear GPIOs según un dígito BCD
	uint8_t bcd_digit = 4; // Ejemplo: 1001 en BCD
	setGpioFromBcd(bcd_digit, gpioConfig);

	uint8_t digits1 = NUM_DIGITS; // Número de dígitos a mostrar

	// Define el vector que mapea los bits BCD a los GPIOs correspondientes
	gpioConf_t gpio_bcd[NUM_PINS] = {
		{GPIO_20, GPIO_OUTPUT},
		{GPIO_21, GPIO_OUTPUT},
		{GPIO_22, GPIO_OUTPUT},
		{GPIO_23, GPIO_OUTPUT}};

	// Define el vector que mapea los dígitos a los GPIOs correspondientes
	gpioConf_t gpio_digits[NUM_DIGITS] = {
		{GPIO_19, GPIO_OUTPUT}, // Dígito 1
		{GPIO_18, GPIO_OUTPUT}, // Dígito 2
		{GPIO_9, GPIO_OUTPUT}	// Dígito 3
	};

	// Configura los GPIOs
	configureGpio(gpio_bcd);
	configureGpio(gpio_digits);

	// Mostrar el valor en el display
	displayValueOnLcd(data, digits1, gpio_digits, gpio_bcd);
}

/*==================[end of file]============================================*/