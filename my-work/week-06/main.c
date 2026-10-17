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

volatile uint8_t contador_de_interrupciones = 0;

void init_gpio();
void exti13_init();
void EXTI15_10_IRQHandler(void);

int main(void)
{
    // Initialize the system
    init_gpio();
    exti13_init();

    while (1)
    {
        // Main loop code here
    }
}

void init_gpio()
{
    /*Habilitamos el reloj del puerto GPIOC*/
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOCEN; // Deshabilitamos el reloj del puerto GPIOC para tener buenas practicas. Esta funcion shiftea el bit del registro AHB1ENR para deshabilitar el reloj del puerto GPIOC, ya que manualmente se haria (1 << 2) y se hace un AND con el registro AHB1ENR para deshabilitar el reloj del puerto GPIOC.
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; // Habilitamos el reloj del puerto GPIOC, esta funcion shiftea el bit del registro AHB1ENR para habilitar el reloj del puerto GPIOC, ya que manualmente se haria (1 << 2) y se hace un OR con el registro AHB1ENR para habilitar el reloj del puerto GPIOC.

    
    /* CONFIGURACION DEL PIN13 del EXTI13*/
    /* Vamos a configurar el PIN13 como entrada simple al pin GPIOC y sin pull-up ni pull-down */
    GPIOC->MODER  &= ~GPIO_MODER_MODE13; // Limpiamos el registro MODER del pin 13 del puerto GPIOC para configurar el pin como entrada. Esta funcion hace un AND con el registro MODER del puerto GPIOC para limpiar los bits correspondientes al pin 13, ya que manualmente se haria (1 << 26) y (1 << 27) y se hace un AND con el registro MODER del puerto GPIOC para limpiar los bits correspondientes al pin 13.
    GPIOC->MODER  &= ~GPIO_MODER_MODER13_Msk; // Limpiamos el registro MODER del pin 13 del puerto GPIOC para configurar el pin como entrada. Esta funcion hace un AND con el registro MODER del puerto GPIOC para limpiar los bits correspondientes al pin 13, ya que manualmente se haria (1 << 26) y (1 << 27) y se hace un AND con el registro MODER del puerto GPIOC para limpiar los bits correspondientes al pin 13.

    GPIOC->PUPDR &= ~GPIO_PUPDR_PUPD13; // Limpiamos el registro PUPDR del pin 13 del puerto GPIOC para configurar el pin como entrada sin pull-up ni pull-down. Esta funcion hace un AND con el registro PUPDR del puerto GPIOC para limpiar los bits correspondientes al pin 13, ya que manualmente se haria (1 << 26) y (1 << 27) y se hace un AND con el registro PUPDR del puerto GPIOC para limpiar los bits correspondientes al pin 13.
    GPIOC->PUPDR &= ~(0b11 << GPIO_PUPDR_PUPD13_Pos); // Limpiamos el registro PUPDR del pin 13 del puerto GPIOC para configurar el pin como entrada sin pull-up ni pull-down. Esta funcion es similar a  `~GPIO_MODER_MODER13_Msk` en donde se hace un AND con el registro PUPDR del puerto GPIOC para limpiar los bits correspondientes al pin 13, ya que manualmente se haria (1 << 26) y (1 << 27) y se hace un AND con el registro PUPDR del puerto GPIOC para limpiar los bits correspondientes al pin 13.
}


void exti13_init()
{
    /* Habilitamos el reloj del SYSCFG */
    RCC->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN; // Deshabilitamos el reloj del SYSCFG para tener buenas practicas. Esta funcion shiftea el bit del registro APB2ENR para deshabilitar el reloj del SYSCFG, ya que manualmente se haria (1 << 14) y se hace un AND con el registro APB2ENR para deshabilitar el reloj del SYSCFG.
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Habilitamos el reloj del SYSCFG, (Equivalente a: |= (1UL << 14))  el EXTIC esta usando el canal APB2 , esta funcion shiftea el bit del registro APB2ENR para habilitar el reloj del SYSCFG, ya que manualmente se haria (1 << 14) y se hace un OR con el registro APB2ENR para habilitar el reloj del SYSCFG.

    /* Configuramos la linea EXTI13 para que sea activada por el pin GPIOC13, es decir encender senal de reloj para EXTI
    Entonces nos dirigimos a `SYSCFG_EXTICR4` ya que es donde se encuentra la configuracion de las lineas de interrupcion del pin 13 */
    SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI13; // Limpiamos el registro EXTICR4 del SYSCFG para configurar la linea EXTI13 para que sea activada por el pin GPIOC13. Esta funcion hace un AND con el registro EXTICR4 del SYSCFG para limpiar los bits correspondientes a la linea EXTI13, ya que manualmente se haria ~(0b1111 << 4) y se hace un AND con el registro EXTICR4 del SYSCFG para limpiar los bits correspondientes a la linea EXTI13.
    SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI13_PC; // Configuramos la linea EXTI13 para que sea activada por el pin GPIOC13. Esta funcion hace un OR con el registro EXTICR4 del SYSCFG para configurar los bits correspondientes a la linea EXTI13, ya que manualmente se haria (1 << 4) y (1 << 5) y se hace un OR con el registro EXTICR4 del SYSCFG para configurar los bits correspondientes a la linea EXTI13.
    //SYSCFG->EXTICR |= (0b0010 << SYSCFG_EXTICR4_EXTI13_Pos); // Configuramos la linea EXTI13 para que sea activada por el pin GPIOC13. Esta funcion hace un OR con el registro EXTICR4 del SYSCFG para configurar los bits correspondientes a la linea EXTI13, ya que manualmente se haria (0b0010 << 4) y se hace un OR con el registro EXTICR4 del SYSCFG para configurar los bits correspondientes a la linea EXTI13.
    
    
    /* Configuramos la interrupcion de la linea EXTI13 para que sea activada por un flanco descendente */
    EXTI->FTSR |= EXTI_FTSR_TR13; // Configuramos la interrupcion de la linea EXTI13 para que sea activada por un flanco descendente. Manualmente se haria (1 << 13) y se hace un OR con el registro FTSR del EXTI.
    EXTI->RTSR &= ~EXTI_RTSR_TR13; // Desactivamos la interrupcion de flanco ascendente, porque en este caso queremos que sea activada por un flanco descendente. Manualmente se haria ~(1 << 13) y se hace un AND con el registro RTSR del EXTI.
    EXTI->SWIER &= ~EXTI_SWIER_SWIER13; // Desactivamos la interrupcion de software, porque en este caso queremos que sea activada por un flanco descendente y la interrupcion por software lo que hace es generar una interrupcion manualmente. Manualmente se haria ~(1 << 13) y se hace un AND con el registro SWIER del EXTI.
    EXTI->IMR |= EXTI_IMR_MR13; // Habilitamos la mascara de interrupcion de la linea EXTI13 para que sea activada por un flanco descendente. Manualmente se haria (1 << 13) y se hace un OR con el registro IMR del EXTI.
    //EXTI->EMR |= EXTI_EMR_MR13; // Habilitamos la mascara de interrupcion de la linea EXTI13 para que sea activada por un flanco descendente. Manualmente se haria (1 << 13) y se hace un OR con el registro EMR del EXTI.
    EXTI->PR |= EXTI_PR_PR13; // Limpiamos el registro PR del EXTI para que no haya interrupciones pendientes de la linea EXTI13, mediante asignacion directa ya que es rc_we, es decir, escribiendo 1 en el bit correspondiente a la linea EXTI13. Manualmente se haria (1 << 13) y se hace un OR con el registro PR del EXTI.
    
    /*Registramos/Matriculamos la interrupcion en IRQ_EXTI13*/
    NVIC_EnableIRQ(EXTI15_10_IRQn); // Habilitamos la interrupcion de la linea EXTI13 en el NVIC. Esta funcion es equivalente a `NVIC->ISER[1] = (1 << (EXTI15_10_IRQn & 0x1F))` y se hace un OR con el registro ISER del NVIC para habilitar la interrupcion de la linea EXTI13.

}



void EXTI15_10_IRQHandler(void)
{


    if(EXTI->PR & EXTI_PR_PR13) // Verificamos si la interrupcion fue generada por la linea EXTI13, es decir, si el bit correspondiente a la linea EXTI13 en el registro PR del EXTI esta en 1. Manualmente se haria (1 << 13) y se hace un AND con el registro PR del EXTI.
    {
        /*Vamos a crear la ISR para EXTI13*/
        /*Primero hay que limpiar el register escribiendo 1 en el bit correspondiente*/
        EXTI->PR |= EXTI_PR_PR13; // Bajamos la bandera.

        /* De forma final para revisar el funcionamiento, vamos a definir una variable que se va a incrementar dentro de la interrupcion*/
        contador_de_interrupciones++;
    }

}

