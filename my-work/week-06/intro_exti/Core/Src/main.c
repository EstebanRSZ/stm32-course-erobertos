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

volatile uint8_t contador_de_interrupciones = 0; // voltaile porque se modifica de forma dinamica sin que el compilador lo optimice, es decir, que no lo guarde en un registro y lo lea de la memoria cada vez que se necesite. Esta variable se va a incrementar dentro de la interrupcion EXTI13_IRQHandler.
volatile uint8_t cambio =0;
uint8_t parpadeo = 0; // Variable para contar los parpadeos del LED verde, es decir, cuantas veces se ha encendido y apagado el LED verde. Se incrementa en la interrupcion del timer TIM2_IRQHandler.

//Cabecera de funciones
void init_gpio();
void exti13_init();
void init_timer();
void EXTI15_10_IRQHandler(void);
void TIM2_IRQHandler(void);


typedef enum {
    Rojo,
    VerdeE,
    VerdeP,
    Amarillo 
} EstadoSemaforo;

EstadoSemaforo estado_actual = Rojo; // Inicializamos el estado actual del semáforo en Rojo, es decir, el primer estado del semáforo.



int main(void)
{
    // Initialize the system
    init_gpio();
    exti13_init();
    init_timer();


    while (1)
    {
        if (cambio ==1) 
        { // Verificamos si la bandera de conteo del SYSTICK está activa, lo que indica que ha transcurrido el tiempo configurado (1 segundo en este caso)
            cambio = 0; // Reiniciamos la bandera de conteo del SYSTICK para esperar el siguiente intervalo de tiempo

            switch (estado_actual) 
            {

                case Rojo: // Estado 1: LED Rojo encendido
                    GPIOB->ODR |= GPIO_ODR_OD6; // Encendemos el LED rojo (pin 6)
                    GPIOB->ODR &= ~GPIO_ODR_OD8; // Apagamos el LED verde (pin 8)
                    GPIOB->ODR &= ~GPIO_ODR_OD9; // Apagamos el LED amarillo (pin 9)
                    estado_actual = VerdeE; // Le sumamos 1 al estado actual para pasar al siguiente estado
                    break;

                case VerdeE: // Estado 2: LED Verde encendido
                    GPIOB->ODR |= GPIO_ODR_OD8; // Encendemos el LED verde (pin 8)
                    GPIOB->ODR &= ~GPIO_ODR_OD9; // Apagamos el LED amarillo (pin 9)
                    GPIOB->ODR &= ~GPIO_ODR_OD6; // Apagamos el LED rojo (pin 6)
                    estado_actual = VerdeP;
                    break;

                case VerdeP: // Estado 3: LED Verde parpadeando
                    GPIOB->ODR ^= GPIO_ODR_OD8; // Cambiamos el estado del LED verde (pin 8) para que parpadee o alterne de estado.
                    parpadeo ++;

                    if (parpadeo ==5*2) {
                        parpadeo = 0; // Reiniciamos el contador de parpadeo a 0 para volver a contar los 5 segundos de parpadeo
                        estado_actual = Amarillo; // Le sumamos 1 al estado actual para pasar al siguiente estado
                    }
                    break;

                case Amarillo: // Estado 4: LED Amarillo encendido
                    
                    GPIOB->ODR |= GPIO_ODR_OD9; // Encendemos el LED amarillo (pin 9)
                    GPIOB->ODR &= ~GPIO_ODR_OD8; // Apagamos el LED verde (pin 8)
                    GPIOB->ODR &= ~GPIO_ODR_OD6; // Apagamos el LED rojo (pin 6)
                    
                    estado_actual = Rojo; // Reiniciamos el estado actual a Rojo para volver al primer estado
                    break;


                default: // Estado 4: Todos los LEDs apagados
                    estado_actual = Rojo; // Reiniciamos el estado actual a 0 para volver al primer estado
                    break;
            
            
            }
        }
    }
}

void init_gpio()
{
    /* Habilitamos el reloj del puerto GPIOA, que es para el Blinky */
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN; 
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; 

    /* Habilitamos el reloj del puerto GPIOB, que son los LEDs*/
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN; 
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; 

    /*Habilitamos el reloj del puerto GPIOC para el pulsador/boton*/
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOCEN; // Deshabilitamos el reloj del puerto GPIOC para tener buenas practicas. Esta funcion shiftea el bit del registro AHB1ENR para deshabilitar el reloj del puerto GPIOC, ya que manualmente se haria (1 << 2) y se hace un AND con el registro AHB1ENR para deshabilitar el reloj del puerto GPIOC.
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; // Habilitamos el reloj del puerto GPIOC, esta funcion shiftea el bit del registro AHB1ENR para habilitar el reloj del puerto GPIOC, ya que manualmente se haria (1 << 2) y se hace un OR con el registro AHB1ENR para habilitar el reloj del puerto GPIOC.


    /* PA5 Blinky*/
    GPIOA->MODER  &= ~GPIO_MODER_MODE5; // Limpiamos el registro MODER del pin 5 del puerto GPIOA para configurar el pin como salida. Esta funcion hace un AND con el registro MODER del puerto GPIOA para limpiar los bits correspondientes al pin 5, ya que manualmente se haria (1 << 10) y (1 << 11) y se hace un AND con el registro MODER del puerto GPIOA para limpiar los bits correspondientes al pin 5.
    GPIOA->MODER  |= GPIO_MODER_MODER5_0; // Configuramos el pin 5 del puerto GPIOA como salida. Esta funcion hace un OR con el registro MODER del puerto GPIOA para configurar el pin 5 como salida, ya que manualmente se haria (1 << 10) y se hace un OR con el registro MODER del puerto GPIOA para configurar el pin 5 como salida.
    GPIOA->OTYPER &= ~GPIO_OTYPER_OT5; // Configuramos el pin 5 del puerto GPIOA como salida push-pull. Esta funcion hace un AND con el registro OTYPER del puerto GPIOA para configurar el pin 5 como salida push-pull, ya que manualmente se haria ~(1 << 5) y se hace un AND con el registro OTYPER del puerto GPIOA para configurar el pin 5 como salida push-pull.
    GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR5; // Configuramos el pin 5 del puerto GPIOA como salida de baja velocidad. Esta funcion hace un AND con el registro OSPEEDR del puerto GPIOA para configurar el pin 5 como salida de baja velocidad, ya que manualmente se haria ~(1 << 10) y ~(1 << 11) y se hace un AND con el registro OSPEEDR del puerto GPIOA para configurar el pin 5 como salida de baja velocidad.
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD5; // Configuramos el pin 5 del puerto GPIOA como salida sin pull-up ni pull-down.
    GPIOA->ODR &= ~GPIO_ODR_OD5; // Inicializamos el pin 5 del puerto GPIOA en bajo, es decir, apagado. Esta funcion hace un AND con el registro ODR del puerto GPIOA para inicializar el pin 5 en bajo, ya que manualmente se haria ~(1 << 5) y se hace un AND con el registro ODR del puerto GPIOA para inicializar el pin 5 en bajo.


    /*Configuracion de los pines de los LEDs, como estan todos en el Puerto B se pueden configurar al tiempo*/
    GPIOB->MODER &= ~(GPIO_MODER_MODE8 | GPIO_MODER_MODE9 | GPIO_MODER_MODE6); // Limpiamos el MODER para todo el puerto B de los pines a usar.
    GPIOB->MODER |= (GPIO_MODER_MODE8_0 | GPIO_MODER_MODE9_0 | GPIO_MODER_MODE6_0); // Configuramos los pines 8, 9 y 6 del puerto GPIOB como salida. Esta funcion hace un OR con el registro MODER del puerto GPIOB para configurar los pines 8, 9 y 6 como salida, ya que manualmente se haria (1 << 16) y (1 << 18) y (1 << 12) y se hace un OR con el registro MODER del puerto GPIOB para configurar los pines 8, 9 y 6 como salida.
    GPIOB->OTYPER &= ~(GPIO_OTYPER_OT8 | GPIO_OTYPER_OT9 | GPIO_OTYPER_OT6); // Configuramos los pines 8, 9 y 6 del puerto GPIOB  como salida push-pull.
    GPIOB->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR8 | GPIO_OSPEEDER_OSPEEDR9 | GPIO_OSPEEDER_OSPEEDR6); // Configuramos los pines 8, 9 y 6 del puerto GPIOB como salida de baja velocidad.
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD8 | GPIO_PUPDR_PUPD9 | GPIO_PUPDR_PUPD6); // Configuramos los pines 8, 9 y 6 del puerto GPIOB como salida sin pull-up ni pull-down.
    GPIOB->ODR &= ~(GPIO_ODR_OD8 | GPIO_ODR_OD9 | GPIO_ODR_OD6); // Inicializamos los pines 8,

    
    /* CONFIGURACION DEL PIN13 que es del BOTON azul para el pulsador, para que sea interrupcion mediante EXTI13*/
    /* Vamos a configurar el PIN13 como entrada simple al pin GPIOC y sin pull-up ni pull-down */
    GPIOC->MODER  &= ~GPIO_MODER_MODE13; // Limpiamos el registro MODER del pin 13 del puerto GPIOC para configurar el pin como entrada. Esta funcion hace un AND con el registro MODER del puerto GPIOC para limpiar los bits correspondientes al pin 13, ya que manualmente se haria (1 << 26) y (1 << 27) y se hace un AND con el registro MODER del puerto GPIOC para limpiar los bits correspondientes al pin 13.
    GPIOC->PUPDR &= ~GPIO_PUPDR_PUPD13; // Limpiamos el registro PUPDR del pin 13 del puerto GPIOC para configurar el pin como entrada sin pull-up ni pull-down. Esta funcion hace un AND con el registro PUPDR del puerto GPIOC para limpiar los bits correspondientes al pin 13, ya que manualmente se haria (1 << 26) y (1 << 27) y se hace un AND con el registro PUPDR del puerto GPIOC para limpiar los bits correspondientes al pin 13.
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
    EXTI->EMR &= ~EXTI_EMR_MR13; // Deshabilitamos la mascara de evento de la linea EXTI13, porque en este caso queremos que sea activada por un flanco descendente y no queremos que genere un evento. Manualmente se haria ~(1 << 13) y se hace un AND con el registro EMR del EXTI.
    EXTI->PR |= EXTI_PR_PR13; // Limpiamos el registro PR del EXTI para que no haya interrupciones pendientes de la linea EXTI13, mediante asignacion directa ya que es rc_we, es decir, escribiendo 1 en el bit correspondiente a la linea EXTI13. Manualmente se haria (1 << 13) y se hace un OR con el registro PR del EXTI.
    
    /*Registramos/Matriculamos la interrupcion en IRQ_EXTI13*/
    NVIC_EnableIRQ(EXTI15_10_IRQn); // Habilitamos la interrupcion de la linea EXTI13 en el NVIC. Esta funcion es equivalente a `NVIC->ISER[1] = (1 << (EXTI15_10_IRQn & 0x1F))` y se hace un OR con el registro ISER del NVIC para habilitar la interrupcion de la linea EXTI13.

}


void init_timer()
{
    // Configuración del temporizador, en el orden solo importa que el enable del timer sea lo ultimo, ya que si se habilita antes de configurar el prescaler y el auto-reload, el timer empezara a contar con valores incorrectos y no se obtendra la frecuencia deseada.
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Habilitamos el reloj del temporizador TIM2
    TIM2->PSC = 16000 - 1; // Prescaler para obtener una frecuencia de 1 kHz (16 MHz / 16000 = 1 kHz)
    TIM2->ARR = 500 - 1; // Auto-reload para obtener un periodo de 500 ms (1 kHz / 500 = 2 Hz), cuando el contador llegue a 500, se levanta la bandera de interrupcion y se reinicia el contador a 0, es decir, se genera un desbordamiento.
    TIM2->CNT = 0; // Inicializamos el contador en 0

    TIM2->DIER |= TIM_DIER_UIE; // Habilitamos la interrupción por desbordamiento del temporizador TIM2
    TIM2->CR1 &= ~TIM_CR1_DIR; // Direccion ascendente
    TIM2->CR1 |= TIM_CR1_ARPE; // Habilitamos el auto-reload para que el contador se reinicie automaticamente cuando llegue al valor del auto-reload, es decir, cuando llegue a 500, se reinicia a 0.

    TIM2->SR &= ~TIM_SR_UIF; // Limpiamos la bandera de interrupción por desbordamiento
    NVIC_EnableIRQ(TIM2_IRQn); // Habilitamos la interrupción del temporizador TIM2 en el NVIC
    TIM2->CR1 |= TIM_CR1_CEN; // Iniciamos el temporizador, es lo ultimo que se habilita

}



void EXTI15_10_IRQHandler(void)
{
 /* El handler tiene que ejecutar codigo lo menos posible, es decir no debe haber tanta logica en el handler */

    if(EXTI->PR & EXTI_PR_PR13) // Verificamos si la interrupcion fue generada por la linea EXTI13, es decir, si el bit correspondiente a la linea EXTI13 en el registro PR del EXTI esta en 1. Manualmente se haria (1 << 13) y se hace un AND con el registro PR del EXTI.
    {
        /*Vamos a crear la ISR para EXTI13*/
        /*Primero hay que limpiar el register escribiendo 1 en el bit correspondiente*/
        EXTI->PR |= EXTI_PR_PR13; // Bajamos la bandera, limpiando el registro PR con un 1 (asi esta definido en el manual del micro)
    

        cambio = 1;
    }

}

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) // Verificamos si la bandera esta subida, es decir si lainterrupción fue generada por el desbordamiento del temporizador TIM2
    {
        TIM2->SR &= ~TIM_SR_UIF; // Bajamos la bandera. Limpiamos la bandera de interrupción por desbordamiento

        GPIOA->ODR ^= GPIO_ODR_OD5; // Cambiamos el estado del semaforo, del pin 13 del puerto GPIOC, es decir, si estaba en alto lo ponemos en bajo y viceversa. Esta funcion hace un XOR con el registro ODR del puerto GPIOC para cambiar el estado del pin 13, ya que manualmente se haria (1 << 13) y se hace un XOR con el registro ODR del puerto GPIOC para cambiar el estado del pin 13.
    }
}
