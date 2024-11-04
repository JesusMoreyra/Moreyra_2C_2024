/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * Examen parcial electrónica programable 2c2024
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * | EDU-ESP    | Periferico|
 * |:----------:|:-----------|
 * |sensor distancia| | 
 * |GPIO_3 | ECHO |
 * |GPIO_5 | TRIGGER |
 * |GPIO_0 | alarma|
 * | GND   | 		GND   |
 * | +5V   | 		+5V   |
 * | CH0   | Acelerometro	| 
 * 

 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Moreyra Jesus (jesusbenja@gmail.com)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "gpio_mcu.h"
#include "timer_mcu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"
#include "switch.h"
#include "hc_sr04.h"
#include "led.h"
#include "ble_mcu.h"

/*==================[macros and definitions]=================================*/
#define CONFIG_PERIOD_medicion_US 500000    //para que me repita la medicion cada 500ms
#define GPIO_ALARMA GPIO_0
/*==================[internal data definition]===============================*/
uint16_t distancia = 0;
uint32_t threshold = 4;
uint16_t valorLectura = 0;
TaskHandle_t deteccion_handle = NULL;
TaskHandle_t notifyBlueTooth_handle = NULL;
/*==================[internal functions declaration]=========================*/

/**
 * @brief Notifica a la tarea asociada para que revise la proximidad
 * @param param puntero a un parámetro que no se utiliza
 */
void FuncTimerDeteccionProximidad(void *param)
{
	vTaskNotifyGiveFromISR(deteccion_handle, pdFALSE); /* Envia una notificacion a la tarea asociada */
}
/**
 * @brief Envía una notificación a la tarea asociada para que envíe los datos por Bluetooth
 * @param param puntero a un parámetro que no se utiliza
 */
void FuncTimerBT(void *param)
{
    vTaskNotifyGiveFromISR(notifyBlueTooth_handle, pdFALSE); /* Envía una notificación a la tarea asociada */
}
/**
 * @brief Lee la distancia del sensor ultrasónico y la almacena en la variable global distancia
 * @param pvParameter puntero a un parámetro que no se utiliza
 * @details La función utiliza la función HcSr04Init para inicializar el sensor,
 *          HcSr04ReadDistanceInCentimeters para leer la distancia y
 *          HcSr04Deinit para desinicializar el sensor.
 *          La distancia se almacena en la variable global distancia.
 *          La función espera una notificación de la tarea asociada para realizar la medicion.
 */
static void Mediciondistancia(void *pvParameter)
{
	while (1)
	{
		ulTaskNotifyTake (pdTRUE, portMAX_DELAY); // Envia una notificacion a la tarea asociada, y hace saber cada cuanto se tiene que realizar la medicion
		HcSr04Init(GPIO_5, GPIO_3);
		distancia = HcSr04ReadDistanceInCentimeters(); //Lectura del sensor
		HcSr04Deinit();
	}
}
/**
 * @brief Control de leds basado en mediciones
 * @param pvParameter puntero a un parámetro que no se utiliza.
 * @details Esta tarea espera una notificación para proceder y luego verifica la variable global 'distancia'. Enciende diferentes LEDs y activa una alarma GPIO según el rango de distancia:

*El LED_1 se enciende si la distancia es menor o igual a 500 cm.
*El LED_2 y la alarma se activan durante 1 segundo si la distancia está entre 300 y 500 cm.
*El LED_3 y la alarma se activan durante 0,5 segundos si la distancia es menor o igual a 300 cm. La tarea luego se retrasa durante 1 segundo antes de la siguiente iteración.

 */
static void LedsTask(void *pvParameter)
{
	while (true)
	{
		ulTaskNotifyTake (pdTRUE, portMAX_DELAY);
		if (distancia <= 500)	
		{
			LedOn(LED_1);
		}
		
		if (distancia >= 300 && distancia <= 500)
		{
			LedOn(LED_2);
			GPIOOn(GPIO_ALARMA);
			vTaskDelay(pdMS_TO_TICKS(1000000)); //PRECAUCION se mantiene encendida por 1 segundo
			GPIOoff(GPIO_ALARMA);
		}
		
		if (distancia <= 300)
		{
			LedOn(LED_3);
			GPIOOn(GPIO_ALARMA);
			vTaskDelay(pdMS_TO_TICKS(500000)); //PELIGRO se mantiene encendida por 0.5 segundos
			GPIOoff(GPIO_ALARMA);
		}

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

/**
 * @brief envia notificaciones a traves de bluetooth
 * @param pvParameter Puntero a un parámetro que no se utiliza.
 * @details La función verifica periódicamente la variable global 'distancia' y envía notificaciones según las siguientes condiciones:

Si la distancia está entre 300 y 500 cm y la alarma está activa, envía una advertencia de "Precaución, vehículo cerca" a través de Bluetooth y el valor de la distancia.
Si la distancia es menor o igual a 300 cm y la alarma está activa, envía una advertencia de "Peligro, vehículo cerca" a través de Bluetooth y el valor de la distancia.
Si no se cumplen las condiciones anteriores, envía una notificación genérica a través de Bluetooth.
 */
static void notifyBT(void *pvParameter)
{
    
    char msg[48];
    while (true)
    {
		vTaskDelay( CONFIG_PERIOD_medicion_US / portTICK_PERIOD_MS);
		if (distancia >= 300 && distancia <= 500)
		{
			if (GPIO_ALARMA == 1)
            {
                BleSendString("*C“Precaución, vehículo cerca\n");
            }
			sprintf(msg, "*H%d \n", (int)distancia);
		}
		if (distancia <= 300)
		{
			if (GPIO_ALARMA == 1)
            {
                BleSendString("*C“Peligro, vehículo cerca”\n");
            }
			sprintf(msg, "*H%d \n", (int)distancia);
		}
         
            else
            BleSendString("*C\n");
    }
}

static void deteccionCaida(void *pvParameter)
{

    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        AnalogInputReadSingle(CH0, &valorLectura);
        UartSendString(UART_PC, (char *)UartItoa(valorLectura, 10));
        UartSendString(UART_PC, "\r\n");

        if (valorLectura > threshold)
        {
           {
                BleSendString("*Caída detectada”\n");
		   }
    }
}
}
/*==================[external functions definition]==========================*/
void app_main(void){

	timer_config_t timer_deteccion_proximidad = {  
		.timer = TIMER_A,
		.period = CONFIG_PERIOD_medicion_US,
		.func_p = FuncTimerDeteccionProximidad,
		.param_p = NULL};
	TimerInit(&timer_deteccion_proximidad);
	TimerStart(timer_deteccion_proximidad.timer);

	// GPIO configuration*
	GPIOInit(GPIO_ALARMA, GPIO_OUTPUT); //inicializo los GPIOs

	 /* Bluetooth configuration */

    ble_config_t ble_configuration = {
        "Alertas de seguridad",

    };
    BleInit(&ble_configuration);

	 analog_input_config_t config;

	// acá debería hacer la configuracion de los 3 canales, pero como hacer el cálculo matemático para la deteccion de caida solo uso un canal a modo de ejemplo
    config.input = CH0;
    config.mode = ADC_SINGLE;
    config.func_p = NULL;
    config.param_p = NULL;
    config.sample_frec = NULL;

    AnalogInputInit(&config);

    serial_config_t my_uart = {
        .port = UART_PC,
        .baud_rate = 115200,
        .func_p = NULL,
        .param_p = NULL};

    UartInit(&my_uart);

  	xTaskCreate(&notifyBT, "Bluetooth", 2048, NULL, 5, &notifyBlueTooth_handle);
	xTaskCreate(&Medicion_proximidad, "Sensado", 512, NULL, 5, &deteccion_handle); 
	xTaskCreate(&deteccionCaida, "Caida", 2048, NULL, 5, NULL);
}
/*==================[end of file]============================================*/