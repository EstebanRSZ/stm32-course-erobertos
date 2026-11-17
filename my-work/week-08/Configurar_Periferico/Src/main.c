/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Esteban Roberto Saiz, erobertos@unal.edu.co
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "stm32f411xe.h"
#include <stm32f4xx.h> // Agregamos la librería de cabecera del microcontrolador STM32F4
#include <stdint.h>

void init_gpio(void);
void init_timer_BLINKY(void);
void init_timer_display(void);

int main(void)
{
    init_gpio();
    init_timer_BLINKY();
    init_timer_display();
	while(1)
    {
        // Logica del programa que se ejecuta siempre
    
    }
}

void init_gpio()
{

    /* Habilitamos el reloj del puerto GPIOH, que es para el Blinky de la Tarjeta didáctica */
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOHEN; 
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN; 

    /* Habilitamos el reloj del puerto GPIOA*/
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN; 
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; 


    /* PH1 Blinky en la Tarjeta didactica, porque sin ella usariamos el PA5*/
    GPIOH->MODER  &= ~GPIO_MODER_MODE1; // Limpiamos el registro MODER del pin 1 del puerto GPIOH para configurar el pin como salida. 
    GPIOH->MODER  |= GPIO_MODER_MODER1_0; // Configuramos el pin 1 del puerto GPIOH como salida. 
    GPIOH->OTYPER &= ~GPIO_OTYPER_OT1; // Configuramos como salida push-pull. 
    GPIOH->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR1; // Configuramos el pin 1 del puerto GPIOH como salida de baja velocidad. 
    GPIOH->ODR &= ~GPIO_ODR_OD1; // Inicializamos el pin 1 del puerto GPIOH en bajo, es decir, apagado. 
    
    /* Configuramos el pin PA0 para usarlo como captura de señal */
    GPIOA->MODER  &= ~GPIO_MODER_MODE5; // 
    GPIOA->MODER  |= GPIO_MODER_MODER5_1; // 10: Alternate function mode


}

void init_timer_BLINKY(void)
{
    /* Vamos a configurar el temporizador TIM11 para el blinky */
    RCC->APB2ENR &= ~RCC_APB2ENR_TIM11EN; // Limpiamos el reloj del timer TIM11
    RCC->APB2ENR |= RCC_APB2ENR_TIM11EN; // Habilitamos el reloj del timer TIM11

    TIM11->PSC = 16000 - 1; // Prescaler para obtener una frecuencia de 1 kHz (16 MHz / 16000 = 1 kHz), el prescaler es como un paso o divisor de frecuencia, es decir, el temporizador va a contar 1 pulso cada 16000 pulsos del reloj del microcontrolador.
    TIM11->ARR = 250 - 1; // Auto-reload para obtener un periodo de 250 ms (1 kHz / 250 = 4 Hz), cuando el contador llegue a 250, se levanta la bandera de interrupcion y se reinicia el contador a 0. Es como el rango del contador, es decir, el temporizador va a contar hasta 250 y luego se reinicia a 0.
    TIM11->CNT = 0; // Inicializamos el contador en 0

    TIM11->DIER |= TIM_DIER_UIE; // Habilitamos la interrupción del temporizador TIM11
    TIM11->CR1 &= ~TIM_CR1_DIR; // Direccion ascendente
    TIM11->CR1 |= TIM_CR1_ARPE; // Habilitamos el auto-reload para que el contador se reinicie automaticamente cuando llegue al valor del auto-reload, es decir, cuando llegue a 250, se reinicia a 0 y luego inicie el conteo nuevo si es el caso.

    TIM11->SR &= ~TIM_SR_UIF; // Limpiamos la bandera de interrupción del temporizador TIM11

    /* Matriculamos la interrupción del temporizador TIM11 que comparte la línea de interrupción del NVIC con TIM1 TRG/COM. */
    NVIC_EnableIRQ(TIM1_TRG_COM_TIM11_IRQn);

    TIM11->CR1 |= TIM_CR1_CEN; // Iniciamos el temporizador, es lo ultimo que se habilita
}

void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
    if (TIM11->SR & TIM_SR_UIF) // Verificamos si la bandera esta subida.
    {
        TIM11->SR &= ~TIM_SR_UIF; // Bajamos la bandera, limpiando el registro.
        GPIOH->ODR ^= GPIO_ODR_OD1; // Conmutamos PH1, donde está conectado el Blinky de la tarjeta didáctica.
    }
}

