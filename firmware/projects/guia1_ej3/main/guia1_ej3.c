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
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Moreyra Jesus
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
/*==================[macros and definitions]=================================*/
#define ON 1
#define OFF 0
#define TOGGLE 3
#define CONFIG_BLINK_PERIOD 100
/*==================[internal data definition]===============================*/
struct leds
{
	uint8_t mode;	  // ON, OFF, TOGGLE
	uint8_t n_led;	  // indica el número de led a controlar
	uint8_t n_ciclos; // indica la cantidad de ciclos de encendido/apagado
	uint16_t periodo; // indica el tiempo de cada ciclo
} my_leds;
/*==================[internal functions declaration]=========================*/
void control_led(struct leds *ledptr)
{
	switch (ledptr->mode)
	{

	case ON:
		switch (ledptr->n_led)
		{
		case 1:
			LedOn(LED_1);
			break;
		case 2:
			LedOn(LED_2);
			break;
		case 3:
			LedOn(LED_3);
			break;
		}
		break;
	case OFF:
		switch (ledptr->n_led)
		{
		case 1:
			LedOff(LED_1);
			break;
		case 2:
			LedOff(LED_2);
			break;
		case 3:
			LedOff(LED_3);
			break;
		}
		break;

	case TOGGLE:
		for (int i = 0; i < ledptr->n_ciclos; i++)
		{
			switch (ledptr->n_led)
			{
			case 1:
				LedToggle(LED_1);
				break;
			case 2:
				LedToggle(LED_2);
				break;
			case 3:
				LedToggle(LED_3);
				break;
			}
			for (int j = 0; j < (ledptr->periodo / CONFIG_BLINK_PERIOD); j++)
				vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
		}
	}
	// return 0;
};
/*==================[external functions definition]==========================*/
void app_main(void)
{
	uint8_t teclas;
	LedsInit();
	SwitchesInit();
	while (1)
	{
		/*teclas = SwitchesRead();
		switch (teclas)
		{
		case SWITCH_1:
			struct leds my_leds = {TOGGLE, 1, 20, 100};
			control_led(&my_leds);
			break;
		case SWITCH_2:
			struct leds my_leds2 = {TOGGLE, 2, 20, 100};
			control_led(&my_leds2);
			break;
		case SWITCH_1 | SWITCH_2:
			struct leds my_leds3 = {TOGGLE, 3, 27, 100};
			control_led(&my_leds3);
			break;
		}*/
	struct leds my_leds3 = {TOGGLE, 3, 10, 500};
			control_led(&my_leds3);
			break;
	}
};

/*==================[end of file]============================================*/