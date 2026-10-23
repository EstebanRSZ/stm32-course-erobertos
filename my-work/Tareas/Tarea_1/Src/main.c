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

/*
    Descripcion del hardware y funcionamiento del sistema
 
  El codigo fue desarrollado en bare-metal, usando los accesos directos a los registros del microcontrolador
  con las definiciones de CMSIS. Use la board de desarrollo Nucleo-F411RE (tiene el microcontrolador STM32F411RE con reloj interno HSI de 16 MHz)
  con una tarjeta didactica donde se puso la protoboard con el circuito. Ademas el boton RESET de la tarjeta didactica 
  permite reiniciar el codigo con presionarlo.
 
  El Hardware utilizado es:
  - El LED de estado (blinky) conectado a PH1 mediante la tarjeta didactica, que permite verificar que el microcontrolador se encuentra en funcionamiento.
  - Un display de 7 segmentos (KEM-3491BSR) que tiene 4 digitos y es de configuracion anodo comun. Donde cada uno de sus siete segmentos 
    tiene una resistencia de 220 ohm para limitar la corriente.

  - Cuatro transistores PNP 2N3906, con resistencias de base de 1 kohm, para
    seleccionar los digitos del display. Los GPIO controlan las bases y los transistores suministran la corriente requerida por cada digito.

  - Dos fotointerruptores, formados internamente por un LED infrarrojo y un fototransistor. El LED infrarrojo se alimenta desde 3,3 V mediante una
    resistencia de 220 ohm y la salida del fototransistor utiliza un pull-up externo de 10 kohm,
    por esto PC0 y PC1 se configuraron sin resistencias pull-up ni pull-down internas.
 
  Conexiones utilizadas:
        PH1: Blinky en la tarjeta didáctica (salida)

        Pines de los 7 segmentos:
        PC5: Segmento A (salida)
        PC8: Segmento B (salida)
        PC12: Segmento C (salida)
        PC11: Segmento D (salida)
        PC10: Segmento E (salida)
        PC6: Segmento F (salida)
        PC13: Segmento G (salida)

        Pines de los transistores para los digitos (1-2-3-4):
        PC2: Transistor Digito 1 (salida)
        PC7: Transistor Digito 2 (salida)
        PC9: Transistor Digito 3 (salida)
        PC3: Transistor Digito 4 (salida)

        Pines de los fotointerruptores:
        PC0: Punto Pull-up con el catodo del Fotointerruptor contador +1 (entrada EXTI0) 
        PC1: Punto Pull-up con el catodo del Fotointerruptor contador -1 (entrada EXTI1)
        

  Funcionamiento e interrupciones:
  - TIM11 genera una interrupcion cada 250 ms y conmuta el LED blinky (PH1). El LED permanece 250 ms encendido y 250 ms apagado,
    por lo que completa un ciclo de parpadeo cada 500 ms, equivalente a 2 Hz.
  - El EXTI0 detecta el flanco ascendente de PC0 y activa evento_incrementar, en la funcion principal se consume el evento y incrementa contador_display en uno.
  - El EXTI1 detecta el flanco descendente de PC1 y activa evento_decrementar, en la funcion principal se consume el evento y decrementa contador_display en uno.
  - El contador se mantiene entre 0 y 9999 para representar como maximo los cuatro digitos disponibles y que por debajo no se genere desbordamiento. Los ceros no significativos no se muestran,
    y el digito de unidades siempre permanece visible.
 
  Maquina de estados del sistema:
  La maquina inicia en ESTADO_INICIO, deja el contador en cero y pasa a ESTADO_ESPERA. En espera no realiza ninguna accion hasta que EXTI0 o EXTI1
  notifica/genera un evento. Luego de generarse el svento, EXTI0 produce la transicion a ESTADO_INCREMENTAR y EXTI1 produce la transicion a ESTADO_DECREMENTAR. 
  El estado seleccionado modifica el contador, actualiza las cifras del display y regresa a ESTADO_ESPERA.

  Refresco del display:
  TIM2 genera una interrupcion independiente cada 1 ms. El handler limpia la bandera del periferico y activa evento_refrescar_display. En el while 
  se revisa fuera de la maquina de estados y se ejecuta el multiplexado, luego el TIM2 nunca modifica estado_actual. Como una vuelta completa tarda 4 ms,
  equivalente a una frecuencia de refresco de 250 Hz , se esta por encima de los 30Hz que el ojo  puede detectar. 
  Ademas se apagan los digitos antes de cambiar los segmentos para evitar el efecto fantasma y, por persistencia de la vision POV
  los cuatro digitos se perciben encendidos simultaneamente como un continuo.
 
  En total se emplean cuatro interrupciones asociadas al funcionamiento: TIM11, TIM2, EXTI0 y EXTI1. 
  Los handlers borran la bandera del respectivo periferico y realizan una accion breve o notifican el evento al programa/parte principal.
*/

#include "stm32f411xe.h"
#include <stm32f4xx.h> // Agregamos la librería de cabecera del microcontrolador STM32F4
#include <stdint.h>


void init_gpio(void);
void init_timer_BLINKY(void);
void init_timer_display(void);

void TIM2_IRQHandler(void);
void TIM1_TRG_COM_TIM11_IRQHandler(void);

void exti0_init(void);
void exti1_init(void);
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);

void apagar_digitos(void);
void apagar_segmentos(void);
void mostrar_numero(uint8_t numero);
void actualizar_digitos(uint16_t numero);
void refrescar_display(void);


/* Estados de la maquina de estados principal del sistema. */
typedef enum
{
    ESTADO_INICIO,
    ESTADO_ESPERA,
    ESTADO_INCREMENTAR,
    ESTADO_DECREMENTAR
} EstadoSistema;

EstadoSistema estado_actual = ESTADO_INICIO;

/* Eventos generados por las interrupciones externas. */
volatile uint8_t evento_incrementar = 0; // volatile porque se modifica en la interrupcion del fotointerruptor de incremento
volatile uint8_t evento_decrementar = 0; // volatile porque se modifica en la interrupcion del fotointerruptor de decremento
volatile uint8_t evento_refrescar_display = 0; // TIM2 solicita un nuevo paso del multiplexado

uint16_t contador_display = 0; // Se almacena el valor del contador que se mostrará en todo el display (todos los 4 digitos) del 7 segmentos, es decir es el numero completo. 


volatile uint8_t digitos_display[4] = {10, 10, 10, 0}; //Cifras que lee TIM2 durante el multiplexado. El valor 10 representa una posicion apagada para ocultar los ceros no significativos, ya que el 10 no representa ningun numero en el display (solo tiene un valor entre 0 y 9).

uint8_t digito_actual = 0;


int main(void)
{
    init_gpio(); // Inicializamos los pines GPIO

    exti0_init(); // Inicializamos la interrupción externa para el fotointerruptor de incremento
    exti1_init(); // Inicializamos la interrupción externa para el fotointerruptor de decremento

    init_timer_BLINKY(); // Inicializamos el temporizador BLINKY
    init_timer_display(); // Inicializamos el temporizador para el display


    
    while (1)
    {
     
        if (evento_refrescar_display == 1)// Si TIM2 notifico un evento de refresco del display, entonces se ejecuta el multiplexado
        {
            evento_refrescar_display = 0;
            refrescar_display();// Se actualiza el digito actual y se pasa al siguiente digito para la proxima interrupcion de TIM2
        }

        switch (estado_actual)
        {
            case ESTADO_INICIO:
                /* Estado ejecutado una sola vez para definir el valor inicial. */
                contador_display = 0;
                actualizar_digitos(contador_display);
                estado_actual = ESTADO_ESPERA;
                break;

            case ESTADO_ESPERA:
                /* La maquina permanece en espera hasta recibir un evento real de alguno de los fotointerruptores. TIM2 no cambia el estado.*/
                if (evento_incrementar == 1)
                {
                    evento_incrementar = 0;
                    estado_actual = ESTADO_INCREMENTAR;
                }
                else if (evento_decrementar == 1)
                {
                    evento_decrementar = 0;
                    estado_actual = ESTADO_DECREMENTAR;
                }
                break;

            case ESTADO_INCREMENTAR:
                if (contador_display < 9999)
                {
                    contador_display++;
                }

                actualizar_digitos(contador_display);
                estado_actual = ESTADO_ESPERA;
                break;

            case ESTADO_DECREMENTAR:
                if (contador_display > 0)
                {
                    contador_display--;
                }

                actualizar_digitos(contador_display);
                estado_actual = ESTADO_ESPERA;
                break;

            default:
                estado_actual = ESTADO_INICIO;
                break;
        }
    }
}


void actualizar_digitos(uint16_t numero)
{
    /*Separamos el numero una sola vez cuando cambia el contador. Las
     posiciones que corresponden a ceros no significativos reciben 10; ese
     valor no representa ningun numero y refrescar_display las deja apagadas.*/

    digitos_display[0] = (numero >= 1000) ? (numero / 1000) % 10 : 10; // Si el numero es mayor o igual a 1000, entonces se calcula el digito de los millares, de lo contrario se asigna 10 para indicar que no hay digito significativo en esa posicion.
    digitos_display[1] = (numero >= 100)  ? (numero / 100)  % 10 : 10;
    digitos_display[2] = (numero >= 10)   ? (numero / 10)   % 10 : 10;
    digitos_display[3] = numero % 10;
}


void refrescar_display(void)
{
    uint8_t numero = digitos_display[digito_actual]; // Obtenemos el numero que corresponde al digito actual (0-3) del display

    apagar_digitos(); //Primero apagamos todos los digitos para evitar el efecto fantasma

    if (numero <= 9)
    {
        mostrar_numero(numero);

        /* Habilitamos solamente el digito que corresponde a esta interrupcion. */
        switch (digito_actual)
        {
            case 0:
                GPIOC->ODR &= ~GPIO_ODR_OD2; // Digito 1: millares
                break;

            case 1:
                GPIOC->ODR &= ~GPIO_ODR_OD7; // Digito 2: centenas
                break;

            case 2:
                GPIOC->ODR &= ~GPIO_ODR_OD9; // Digito 3: decenas
                break;

            case 3:
                GPIOC->ODR &= ~GPIO_ODR_OD3; // Digito 4: unidades
                break;

            default:
                digito_actual = 0;
                apagar_segmentos();
                return;
        }
    }
    else
    {
        /* Posicion sin cifra significativa: segmentos y digito apagados. */
        apagar_segmentos();
    }

    digito_actual++; // Pasamos al siguiente digito para la proxima interrupcion de TIM2
    if (digito_actual >= 4)
    {
        digito_actual = 0; // Reiniciamos el digito_actual a 0 para que vuelva a mostrar el primer digito en la siguiente interrupcion
    }
}


void init_gpio()
{

    /* Habilitamos el reloj del puerto GPIOH, que es para el Blinky de la Tarjeta didáctica */
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOHEN; 
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN; 

    /* Habilitamos el reloj del puerto GPIOC, que es el canal en comun donde se encuentran los 7 Segmentos, Transistores y Fotointerruptores*/
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOCEN; 
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; 

    /* PH1 Blinky en la Tarjeta didactica, porque sin ella usariamos el PA5*/
    GPIOH->MODER  &= ~GPIO_MODER_MODE1; // Limpiamos el registro MODER del pin 1 del puerto GPIOH para configurar el pin como salida. 
    GPIOH->MODER  |= GPIO_MODER_MODER1_0; // Configuramos el pin 1 del puerto GPIOH como salida. 
    GPIOH->OTYPER &= ~GPIO_OTYPER_OT1; // Configuramos como salida push-pull. 
    GPIOH->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR1; // Configuramos el pin 1 del puerto GPIOH como salida de baja velocidad. 
    GPIOH->ODR &= ~GPIO_ODR_OD1; // Inicializamos el pin 1 del puerto GPIOH en bajo, es decir, apagado. 
    

    /*Configuracion de los pines tipo salida para los: 7 Segmentos y Transistores. Como estan todos en el Puerto C se pueden configurar al tiempo los 11 pines*/
    GPIOC->MODER &= ~(GPIO_MODER_MODE10 | GPIO_MODER_MODE11 | GPIO_MODER_MODE12 | GPIO_MODER_MODE13 | GPIO_MODER_MODE5 | GPIO_MODER_MODE6 | GPIO_MODER_MODE8 | GPIO_MODER_MODE3 | GPIO_MODER_MODE2 | GPIO_MODER_MODE7 | GPIO_MODER_MODE9); // Limpiamos el MODER para todo el puerto C de los pines a usar como salida.
    GPIOC->MODER |= (GPIO_MODER_MODE10_0 | GPIO_MODER_MODE11_0 | GPIO_MODER_MODE12_0 | GPIO_MODER_MODE13_0 | GPIO_MODER_MODE5_0 | GPIO_MODER_MODE6_0 | GPIO_MODER_MODE8_0 | GPIO_MODER_MODE3_0 | GPIO_MODER_MODE2_0 | GPIO_MODER_MODE7_0 | GPIO_MODER_MODE9_0 ); // Configuramos los pines del puerto GPIOB que vamos a usar como salida. 
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT10 | GPIO_OTYPER_OT11 | GPIO_OTYPER_OT12 | GPIO_OTYPER_OT13 | GPIO_OTYPER_OT5 | GPIO_OTYPER_OT6 | GPIO_OTYPER_OT8 | GPIO_OTYPER_OT3 | GPIO_OTYPER_OT2 | GPIO_OTYPER_OT7 | GPIO_OTYPER_OT9); // Configuramos los pines del puerto GPIOC como salida push-pull.
    GPIOC->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR10 | GPIO_OSPEEDER_OSPEEDR11 | GPIO_OSPEEDER_OSPEEDR12 | GPIO_OSPEEDER_OSPEEDR13 | GPIO_OSPEEDER_OSPEEDR5 | GPIO_OSPEEDER_OSPEEDR6 | GPIO_OSPEEDER_OSPEEDR8 | GPIO_OSPEEDER_OSPEEDR3 | GPIO_OSPEEDER_OSPEEDR2 | GPIO_OSPEEDER_OSPEEDR7 | GPIO_OSPEEDER_OSPEEDR9); // Configuramos los pines del puerto GPIOC como salida de baja velocidad.
    GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD10 | GPIO_PUPDR_PUPD11 | GPIO_PUPDR_PUPD12 | GPIO_PUPDR_PUPD13 | GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD8 | GPIO_PUPDR_PUPD3 | GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD7 | GPIO_PUPDR_PUPD9); // Configuramos los pines del puerto GPIOC como salida sin pull-up ni pull-down.
    GPIOC->ODR |= (GPIO_ODR_OD10 | GPIO_ODR_OD11 | GPIO_ODR_OD12 | GPIO_ODR_OD13 | GPIO_ODR_OD5 | GPIO_ODR_OD6 | GPIO_ODR_OD8 | GPIO_ODR_OD3 | GPIO_ODR_OD2 | GPIO_ODR_OD7 | GPIO_ODR_OD9); // Inicializamos los pines en 1, ya que en nuestro caso seria estado "Apagado" porque estaria al mismo potencial y no enciende.

    
    /*Configuracion de los pines tipo entrada para los: Fotointerruptores. Como estan los dos en el Puerto C se pueden configurar al tiempo*/
    GPIOC->MODER &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1 ); // Limpiamos el MODER de los pines PC0 y PC1 que vamos a usar como entrada.
    GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD1 ); // Configuramos el PUPDR de los pines PC0 y PC1 como salida sin pull-up ni pull-down, porque tenemos un pull-up externo en el fotointerruptor.
   
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

void init_timer_display(void)
{
    /*Inicializamos el TIM2 que se encuentra en el periférico APB1, para el control del display de 7 segmentos */
    RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN; // Limpiamos el reloj del timer TIM2
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Habilitamos el reloj del timer TIM2

    TIM2->PSC = 1600 - 1; // Prescaler para obtener una frecuencia de 10 kHz (16 MHz / 1600 = 10 kHz = 0.1ms)
    TIM2->ARR = 10- 1; // Auto-reload para obtener un periodo de 1 ms (10 pulsos de 0,1 ms = 1 ms), cuando el contador llegue a 10, se levanta la bandera de interrupcion y se reinicia el contador a 0. SI queremos que se vea mas lento la actualizacion entonces podemos aumentar el valor del auto-reload, por ejemplo, si ponemos 1000, entonces el periodo seria de 100 ms (10 pulsos de 10 ms = 100 ms), y asi sucesivamente.
    TIM2->CNT = 0; // Inicializamos el contador en 0

    TIM2->DIER |= TIM_DIER_UIE; // Habilitamos la interrupción del temporizador TIM2
    TIM2->CR1 &= ~TIM_CR1_DIR; // Direccion ascendente
    TIM2->CR1 |= TIM_CR1_ARPE; // Habilitamos el auto-reload para que el contador se reinicie automaticamente cuando llegue al valor del auto-reload, es decir, cuando llegue a 10, se reinicia a 0.

    TIM2->SR &= ~TIM_SR_UIF; // Limpiamos la bandera de interrupción del temporizador TIM2


    /* Matriculamos la interrupción del temporizador TIM2*/
    NVIC_EnableIRQ(TIM2_IRQn);

    TIM2->CR1 |= TIM_CR1_CEN; // Iniciamos el temporizador, es lo ultimo que se habilita
}

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) // Verificamos si la bandera esta subida.
    {
        TIM2->SR &= ~TIM_SR_UIF; // Bajamos la bandera, limpiando el registro.
        evento_refrescar_display = 1; // Notificamos al while sin modificar estado_actual.
    }
}

void exti0_init()
{
    /* Habilitamos el reloj del SYSCFG */
    RCC->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN; // Deshabilitamos el reloj del SYSCFG para tener buenas practicas. 
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Habilitamos el reloj del SYSCFG, el EXTIC esta usando el canal APB2.

    /* Configuramos la linea EXTI0 para que sea activada por el pin GPIOC0, es decir encender senal de reloj para EXTI
    Entonces nos dirigimos a `SYSCFG_EXTICR1` ya que es donde se encuentra la configuracion de las lineas de interrupcion del pin 0 */
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0; // Limpiamos el registro EXTICR1 del SYSCFG para configurar la linea EXTI0 para que sea activada por el pin GPIOC0. 
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PC; // Configuramos la linea EXTI0 para que sea activada por el pin GPIOC0. 
    
    /* Configuramos la interrupcion de la linea EXTI0 para que sea activada por un flanco ascendente */
    EXTI->FTSR &= ~EXTI_FTSR_TR0; // Desactivamos la interrupcion de la linea EXTI0 para que sea activada por un flanco ascendente. 
    EXTI->RTSR |= EXTI_RTSR_TR0; // Configuramos la interrupcion de flanco ascendente, porque  al interrumpir elfototransistor deja de conducir y elpull-up lleva la entrada a 1 luego el haz se produce un flanco ascendente.
    EXTI->SWIER &= ~EXTI_SWIER_SWIER0; // Desactivamos la interrupcion de software, porque en este caso queremos que sea activada por un flanco ascendente y la interrupcion por software lo que hace es generar una interrupcion manualmente.
    
    EXTI->IMR |= EXTI_IMR_MR0; // Habilitamos la mascara de interrupcion de la linea EXTI0 para que sea activada por un flanco ascendente.
    EXTI->EMR &= ~EXTI_EMR_MR0; // Deshabilitamos la mascara de evento de la linea EXTI0, porque en este caso queremos que sea activada por un flanco ascendente y no queremos que genere un evento.
    EXTI->PR |= EXTI_PR_PR0; // Limpiamos el registro PR del EXTI para que no haya interrupciones pendientes de la linea EXTI0.
    
    /*Registramos/Matriculamos la interrupcion en IRQ_EXTI0*/
    NVIC_EnableIRQ(EXTI0_IRQn); // Habilitamos la interrupcion de la linea EXTI0 en el NVIC. 

}


void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR0)
    {
        EXTI->PR |= EXTI_PR_PR0; // Bajamos la bandera, limpiando el registro PR con un 1 (asi esta definido en el manual del micro)
        evento_incrementar = 1; // Indicamos que se ha generado un evento de incremento
    }
}


void exti1_init()
{
    /* Nos aseguramos de habilitar el reloj del SYSCFG, que ya estaria habilitada por EXTI0, y no se apaga ya que si se deshabilita el SYSCFG se deshabilita el reloj del EXTI y no funcionaria la interrupcion.
    RCC->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN; // Deshabilitamos el reloj del SYSCFG
    */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Habilitamos (En este caso nos aseguramos de que este encendido) el reloj del SYSCFG, el EXTI esta usando el canal APB2.
    

    /* Configuramos la linea EXTI1 para que sea activada por el pin GPIOC1, es decir encender senal de reloj para EXTI
    Entonces nos dirigimos a `SYSCFG_EXTICR1` ya que es donde se encuentra la configuracion de las lineas de interrupcion del pin 1 */
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI1; // Limpiamos el registro EXTICR1 del SYSCFG para configurar la linea EXTI1 para que sea activada por el pin GPIOC1. 
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI1_PC; // Configuramos la linea EXTI1 para que sea activada por el pin GPIOC1.     
    
    /* Configuramos la interrupcion de la linea EXTI1 para que sea activada por un flanco descendente */
    EXTI->FTSR |= EXTI_FTSR_TR1; // Configuramos la interrupcion de la linea EXTI1 para que sea activada por un flanco descendente. 
    EXTI->RTSR &= ~EXTI_RTSR_TR1; // Desactivamos la interrupcion de flanco descendente, porque  al interrumpir elfototransistor deja de conducir y elpull-up lleva la entrada a 1 luego el haz se produce un flanco descendente.
    EXTI->SWIER &= ~EXTI_SWIER_SWIER1; // Desactivamos la interrupcion de software, porque en este caso queremos que sea activada por un flanco descendente y la interrupcion por software lo que hace es generar una interrupcion manualmente.
    
    EXTI->IMR |= EXTI_IMR_MR1; // Habilitamos la mascara de interrupcion de la linea EXTI1 para que sea activada por un flanco descendente.
    EXTI->EMR &= ~EXTI_EMR_MR1; // Deshabilitamos la mascara de evento de la linea EXTI1, porque en este caso queremos que sea activada por un flanco descendente y no queremos que genere un evento.
    EXTI->PR |= EXTI_PR_PR1; // Limpiamos el registro PR del EXTI para que no haya interrupciones pendientes de la linea EXTI1.
    
    /*Registramos/Matriculamos la interrupcion en IRQ_EXTI1*/
    NVIC_EnableIRQ(EXTI1_IRQn); // Habilitamos la interrupcion de la linea EXTI1 en el NVIC. 

}


void EXTI1_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR1) // Revisamos si la bandera esta subida, en este caso, si la interrupcion fue generada por el fotointerruptor de decremento.
    {
        EXTI->PR |= EXTI_PR_PR1; //Bajamos la bandera, limpiando el registro PR con un 1 (asi esta definido en el manual del micro)
        evento_decrementar = 1; // Indicamos que se ha generado un evento de decremento
    }
}


void apagar_digitos(void)
{
    /* Para apagar los digitos se quiere que los PNP esten en 1 (activos en bajo), un 1 apaga el transistor, ya que estaria la base al mismo potencial que el emisor y no conduce. */
     
    GPIOC->ODR |= (GPIO_ODR_OD2 | GPIO_ODR_OD7 | GPIO_ODR_OD9 | GPIO_ODR_OD3);
}

void apagar_segmentos(void)
{
    /* Como tenemos un display de 7 segmentos de ánodo común, para apagar los segmentos queremos que los PNP esten en 1 (activos en bajo),
       un 1 apaga el segmento, ya que estaria el ánodo al mismo potencial que el cátodo (Polarizacion inversa) y no conduce.
     */
    GPIOC->ODR |= (GPIO_ODR_OD5  | GPIO_ODR_OD8  | GPIO_ODR_OD12 | GPIO_ODR_OD11 | GPIO_ODR_OD10 | GPIO_ODR_OD6  | GPIO_ODR_OD13);
}

void mostrar_numero(uint8_t numero)
{
    apagar_segmentos();

    switch (numero)
    {
        case 0:
            /* Encender A, B, C, D, E y F. */
            GPIOC->ODR &= ~(GPIO_ODR_OD5  | GPIO_ODR_OD8  | GPIO_ODR_OD12 | GPIO_ODR_OD11 | GPIO_ODR_OD10 | GPIO_ODR_OD6);
            break;

        case 1:
            /* Encender B y C. */
            GPIOC->ODR &= ~(GPIO_ODR_OD8 | GPIO_ODR_OD12);
            break;

        case 2:
            /* Encender A, B, D, E y G. */
            GPIOC->ODR &= ~(GPIO_ODR_OD5  | GPIO_ODR_OD8  | GPIO_ODR_OD11 | GPIO_ODR_OD10 | GPIO_ODR_OD13);
            break;
        
        case 3:
            /* Encender A, B, C, D y G. */
            GPIOC->ODR &= ~(GPIO_ODR_OD5  | GPIO_ODR_OD8  | GPIO_ODR_OD12 | GPIO_ODR_OD11 | GPIO_ODR_OD13);
            break;
        
        case 4:
            /* Encender B, C, F y G. */
            GPIOC->ODR &= ~(GPIO_ODR_OD8  | GPIO_ODR_OD12 | GPIO_ODR_OD6  | GPIO_ODR_OD13);
            break;  
        
        case 5:
            /* Encender A, C, D, F y G. */
            GPIOC->ODR &= ~(GPIO_ODR_OD5  | GPIO_ODR_OD12 | GPIO_ODR_OD11 | GPIO_ODR_OD6  | GPIO_ODR_OD13);
            break;

        case 6:
            /* Encender A, C, D, E, F y G. */
            GPIOC->ODR &= ~(GPIO_ODR_OD5  | GPIO_ODR_OD12 | GPIO_ODR_OD11 | GPIO_ODR_OD10 | GPIO_ODR_OD6  | GPIO_ODR_OD13);
            break;

        case 7:
            /* Encender A, B y C. */
            GPIOC->ODR &= ~(GPIO_ODR_OD5  | GPIO_ODR_OD8  | GPIO_ODR_OD12);
            break;
                
        case 8:
            /* Encender A, B, C, D, E, F y G. */
            GPIOC->ODR &= ~(GPIO_ODR_OD5  | GPIO_ODR_OD8  | GPIO_ODR_OD12 | GPIO_ODR_OD11 | GPIO_ODR_OD10 | GPIO_ODR_OD6  | GPIO_ODR_OD13);
            break;
        
        case 9:
            /* Encender A, B, C, D, F y G. */
            GPIOC->ODR &= ~(GPIO_ODR_OD5  | GPIO_ODR_OD8  | GPIO_ODR_OD12 | GPIO_ODR_OD11 | GPIO_ODR_OD6  | GPIO_ODR_OD13);
            break;

        default:
            apagar_segmentos();
            break;
    }
}
