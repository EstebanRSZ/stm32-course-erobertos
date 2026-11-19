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
  - LED RGB donde cada uno de sus anodos son conectados a resistencias de 1k Ohms.
  - Amplificador Operacional MCP6004 en modo Buffer/seguidor de voltaje con una resistencia de 220ohms en la salida con un extremo libre.
  - Capacitores 104 (100nF) y Resistencias de 3.3k ohms para filtros pasa bajos.
  - Encoder
  - Jumper para el pin de captura de frecuencia (PA5) que permite medir la frecuencia de una señal externa.

  Conexiones utilizadas:
        PH1: Blinky en la tarjeta didáctica (salida)

        Pines del Encoder:
        - CLK: PA0 (TIM5_CH1, AF2)
        - DT: PA1 (TIM5_CH2, AF2)
        - SW: PC0 (EXTI0, flanco de bajada)

        Pines del RGB:
        - Rojo: PA6 (TIM3_CH1, AF2)
        - Verde: PA7 (TIM3_CH2, AF2)
        - Azul: PC8 (TIM3_CH3, AF2)

        Pin del DAC: PC9 (TIM3_CH4, AF2)

        Pin para captura: PA5 (TIM2_CH1, AF1)


  Funcionamiento e interrupciones:
  - TIM11 genera una interrupcion cada 250 ms y conmuta el LED blinky (PH1). El LED permanece 250 ms encendido y 250 ms apagado,
    por lo que completa un ciclo de parpadeo cada 500 ms, equivalente a 2 Hz.
  - TIM3 genera los 4 PWM (R, G, B y DAC). Comparten ARR, es decir, la misma frecuencia, y cada canal tiene su propio CCR (duty).
  - TIM5 (32 bits) en modo encoder lee CLK y DT con filtro. No genera interrupcion: la FSM lo consulta.
  - TIM2 (32 bits) en captura mide la frecuencia de la señal que entra por PA5.
  - EXTI0 atiende el boton SW del encoder (PC0): su ISR solo sube button_edge_flag (primer flanco de bajada, todavia no es una pulsacion confirmada).
  - TIM4 (Timer_E) hace el debounce del boton SW. Es un temporizador de un solo disparo (one-pulse) de 30 ms: la ISR solo sube debounce_flag
    al vencer. Quien decide es la FSM: al recibir el primer flanco bloquea EXTI0 y arranca TIM4 (los rebotes se ignoran); al vencer la ventana
    vuelve a habilitar EXTI0 y, si PC0 sigue en bajo (el boton sigue presionado), cambia de modo. Si PC0 ya esta en alto fue ruido.

  Resumen de timers: TIM11 (Timer_A, blinky), TIM3 (Timer_B, 4 PWM), TIM2 (Timer_C, captura de frecuencia),
  TIM5 (Timer_D, encoder) y TIM4 (Timer_E, debounce del SW1).

  Por tanto en el codigo se tiene que las ISR "solo" suben banderas y la FSM las convierte en eventos


  ===================== FSM =====================

  STATE_IDLE es el unico estado que lee eventos. Cada evento lleva a un estado que hace UNA tarea y vuelve a IDLE.

  Estado actual         Evento / condicion   Estado siguiente    Accion [ENTER]
  --------------------  -------------------  ------------------  ---------------------------------------------
  STATE_IDLE            EV_BUTTON_EDGE       STATE_DEBOUNCE_START -
  STATE_IDLE            EV_DEBOUNCE_DONE     STATE_DEBOUNCE_CHECK -
  STATE_IDLE            EV_ENCODER_CHANGED   STATE_UPDATE_PWM    -
  STATE_IDLE            EV_BLINK_TICK        STATE_BLINK         -
  STATE_IDLE            EV_CAPTURE_READY     STATE_UPDATE_FREQ   -
  STATE_DEBOUNCE_START  (incondicional)      STATE_IDLE          button_StartDebounce(): bloquea EXTI0 y arranca TIM4
  STATE_DEBOUNCE_CHECK  PC0 sigue en bajo    STATE_NEXT_MODE     button_EndDebounce(): vuelve a habilitar EXTI0
  STATE_DEBOUNCE_CHECK  PC0 ya esta en alto  STATE_IDLE          button_EndDebounce() (era ruido)
  STATE_NEXT_MODE       (incondicional)      STATE_IDLE          edit_mode pasa al siguiente (R, G, B, RGB, DAC)
  STATE_UPDATE_PWM      (incondicional)      STATE_IDLE          pwm_ApplySteps(encoder_TakeSteps())
  STATE_UPDATE_FREQ     (incondicional)      STATE_IDLE          freq_Update()
  STATE_BLINK           (incondicional)      STATE_IDLE          led_Toggle()
  STATE_ERROR           (cualquiera)         STATE_IDLE          recuperacion

  Prioridad en get_next_event(): primer flanco del boton > fin del debounce > encoder > captura > blinky.

*/

#include "stm32f411xe.h"
#include <stm32f4xx.h> 
#include <stdint.h>

/* Constantes del PWM */
#define PWM_PSC             0      // Prescaler del PWM. Con PSC = 0 (tick de 1 us) y ARR = 999 son periodos de 1000 ticks de 1 us (frecuencia de 16 kHz)
#define PWM_ARR             999 // Valor del periodo del PWM. Con PSC = 0 (tick de 1 us) y ARR = 999 son periodos de 1000 ticks de 1 us (frecuencia de 16 kHz)
#define PWM_DUTY_MAX        (PWM_ARR + 1)
#define PWM_DUTY_STEP       10      // Cuanto cambia el CCR por cada paso del encoder
#define ENCODER_COUNTS_STEP 4       // Modo encoder 3: 4 cuentas de CNT por paso mecanico
#define TIMER_CLK_HZ        16000000UL // Frecuencia del reloj del timer (16 MHz)
#define CAPTURE_PSC         0       // PSC de TIM2 en captura: 0 -> 1 tick = 62.5 ns
#define CAPTURE_TICK_HZ     (TIMER_CLK_HZ / (CAPTURE_PSC + 1))
#define DEBOUNCE_MS         30      // Duracion de la ventana de debounce de TIM4 (1 tick = 1 ms), mayor que el tiempo tipico de rebote del boton


/* Estados de la FSM (el QUE) */
typedef enum {
    STATE_IDLE,         // Reposo: unico estado que lee eventos
    STATE_DEBOUNCE_START, // Atiende el primer flanco del boton: bloquea EXTI0 y arranca la ventana de TIM4
    STATE_DEBOUNCE_CHECK, // Atiende el fin de la ventana: habilita EXTI0 y confirma si el boton sigue presionado
    STATE_NEXT_MODE,    // Atiende la pulsacion confirmada: cambia el color/canal que edita el encoder
    STATE_UPDATE_PWM,   // Atiende el giro del encoder: modifica el CCR del modo activo
    STATE_UPDATE_FREQ,  // Atiende una captura nueva: calcula la frecuencia
    STATE_BLINK,        // Atiende el tick de TIM11: conmuta PH1
    STATE_ERROR,        // Estado invalido: se recupera hacia IDLE
    NUM_STATES          // Para validar el rango de estados
} State_t;

/* Modos de edicion: que canal modifica el encoder */
typedef enum {
    MODE_RED,
    MODE_GREEN,
    MODE_BLUE,
    MODE_RGB,// Los tres colores a la vez
    MODE_DAC,
    NUM_MODES// Para validar el rango de modos
} Mode_t;

/* Eventos de la FSM */
typedef enum {
    EV_NONE,
    EV_BUTTON_EDGE,     // Primer flanco de bajada del boton (aun puede ser un rebote)
    EV_DEBOUNCE_DONE,   // Vencio la ventana de debounce de TIM4
    EV_ENCODER_CHANGED,
    EV_CAPTURE_READY,
    EV_BLINK_TICK
} Event_t;


/* Variables globales */
State_t current_state = STATE_IDLE; // Estado actual de la FSM, inicia en estado de reposo
Mode_t edit_mode = MODE_RED; // Modo de edicion actual, que canal modifica el encoder. Inicia en rojo.
uint8_t state_entry_flag = 1;   // Bandera que indica si se acaba de entrar a un estado.

volatile uint8_t button_edge_flag = 0; // La sube la ISR de EXTI0 en el flanco de bajada del boton 
volatile uint8_t debounce_flag = 0;   // La sube la ISR de TIM4 al vencer la ventana de debounce 
volatile uint8_t capture_flag = 0;    // La sube la ISR de TIM2 
volatile uint8_t blink_flag = 0;      // La sube la ISR de TIM11 

volatile uint32_t capture_last = 0;   // Ultimo flanco capturado: solo lo usa la ISR de TIM2 
volatile uint32_t capture_period = 0; // Periodo en ticks entre los dos ultimos flancos: la ISR lo escribe, y en main se lee 

volatile uint32_t freq_hz = 0;        // Frecuencia medida: variable para ver en Live Watch de VS Code 

int32_t duty_red = 0; // Valor del duty del canal rojo (CCR1 de TIM3), inicia en 0 (apagado)
int32_t duty_green = 0; // Valor del duty del canal verde (CCR2 de TIM3), inicia en 0 (apagado)
int32_t duty_blue = 0; // Valor del duty del canal azul (CCR3 de TIM3), inicia en 0 (apagado)
int32_t duty_dac = 0;  // Valor del duty del canal DAC (CCR4 de TIM3), inicia en 0 (apagado)


/* Inicializacion del hardware */
void init_gpio(void);
void init_timer_BLINKY(void);
void init_timer3_pwm(void);
void init_tim5_encoder(void);
void init_tim2_capture(void);
void init_tim4_debounce(void);
void exti0_init(void);

/* ISR */
void TIM1_TRG_COM_TIM11_IRQHandler(void);
void EXTI0_IRQHandler(void);
void TIM2_IRQHandler(void);
void TIM4_IRQHandler(void);

/* Funcion privada del modulo */
static int32_t pwm_Saturate(int32_t duty); // Limita el duty entre 0 y PWM_DUTY_MAX


/* Drivers de la FSM (el COMO) */
Event_t get_next_event(void); // Se obtiene el siguiente evento de la FSM
int32_t encoder_TakeSteps(void); // Devuelve los pasos completos acumulados (+ horario, - antihorario) y los descuenta del contador TIM5
void pwm_ApplySteps(int32_t steps); // Aplica los pasos del encoder al duty del modo activo (edit_mode) y escribe los CCR de TIM3
void freq_Update(void); // Convierte el periodo capturado en frecuencia y la guarda en freq_hz
void led_Toggle(void); // Conmuta el estado del LED PH1
void button_StartDebounce(void);    // Bloquea EXTI0 y arranca la ventana de TIM4
void button_EndDebounce(void);      // Vuelve a habilitar EXTI0 al terminar la ventana
uint8_t button_IsPressed(void);     // 1 si PC0 esta en bajo (boton presionado)


int main(void)
{
    Event_t current_event = EV_NONE;

    init_gpio();
    init_tim4_debounce(); // la ISR del boton usa TIM4, que debe estar listo
    exti0_init();
    init_timer3_pwm();
    init_tim5_encoder();
    init_tim2_capture();
    init_timer_BLINKY();  // Ultimo porque las interrupciones del blinky empiezan con todo el hardware listo 

    while(1)
    {
        /* Seguridad, si el estado esta fuera de rango, se recupera por STATE_ERROR */
        if (current_state >= NUM_STATES) // Verificamos si el estado actual es mayor o igual al número de estados definidos.
        {
            current_state = STATE_ERROR;
            state_entry_flag = 1; // Subimos la bandera 
        }

        switch (current_state)
        {
        case STATE_IDLE: // Estado de reposo
            if (state_entry_flag)
            {
                state_entry_flag = 0; // Bajamos la bandera
            }

            current_event = get_next_event(); // Obtenemos el siguiente evento de la FSM

            if (current_event == EV_BUTTON_EDGE) // Primer flanco del boton: aun no es una pulsacion confirmada
            {
                current_state = STATE_DEBOUNCE_START; // Cambiamos al estado que inicia el debounce
                state_entry_flag = 1; // Subimos la bandera hacia DEBOUNCE_START
            }
            else if (current_event == EV_DEBOUNCE_DONE) // Vencio la ventana de debounce
            {
                current_state = STATE_DEBOUNCE_CHECK; // Cambiamos al estado que confirma la pulsacion
                state_entry_flag = 1; // Subimos la bandera hacia DEBOUNCE_CHECK
            }
            else if (current_event == EV_ENCODER_CHANGED) // Evento de cambio en el encoder
            {
                current_state = STATE_UPDATE_PWM; // Cambiamos al estado de actualización del PWM
                state_entry_flag = 1; // Subimos la bandera hacia UPDATE_PWM
            }
            else if (current_event == EV_CAPTURE_READY) // Evento de captura lista
            {
                current_state = STATE_UPDATE_FREQ; // Cambiamos al estado de actualización de frecuencia
                state_entry_flag = 1; // Subimos la bandera hacia UPDATE_FREQ
            }
            else if (current_event == EV_BLINK_TICK) // Evento del Blinky
            {
                current_state = STATE_BLINK; // Estado de parpadeo
                state_entry_flag = 1; // Subimos la bandera hacia BLINK
            }
            break;

        case STATE_DEBOUNCE_START: // Estado que inicia la ventana de debounce
            if (state_entry_flag)
            {
                button_StartDebounce(); // Bloqueamos EXTI0 (los rebotes ya no interrumpen) y arrancamos TIM4
                state_entry_flag = 0; // Bajamos la bandera
            }

            current_state = STATE_IDLE; // Volvemos al estado de reposo mientras corre la ventana (no se bloquea)
            state_entry_flag = 1; // Subimos la bandera hacia IDLE
            break;

        case STATE_DEBOUNCE_CHECK: // Estado que confirma la pulsacion al terminar la ventana
            if (state_entry_flag)
            {
                button_EndDebounce(); // Volvemos a habilitar EXTI0
                state_entry_flag = 0; // Bajamos la bandera
            }

            if (button_IsPressed()) // PC0 sigue en bajo: el boton esta presionado, la pulsacion es valida
            {
                current_state = STATE_NEXT_MODE; // Cambiamos al estado de siguiente modo
            }
            else // PC0 ya esta en alto: fue ruido
            {
                current_state = STATE_IDLE; // Volvemos al estado de reposo sin cambiar de modo
            }
            state_entry_flag = 1; // Subimos la bandera hacia el estado siguiente
            break;

        case STATE_NEXT_MODE: // Estado de cambio de modo
            if (state_entry_flag)
            {
                edit_mode++; // Incrementamos el modo de edición, pasamos al siguiente color/canal que edita el encoder

                // Esta parte es de seguridad, si edit_mode supera el número de modos definidos, lo reiniciamos a MODE_RED
                if (edit_mode >= NUM_MODES) 
                {
                    edit_mode = MODE_RED;
                }
                state_entry_flag = 0; // Bajamos la bandera
            }

            current_state = STATE_IDLE; // Volvemos al estado de reposo
            state_entry_flag = 1; // Subimos la bandera hacia IDLE
            break;

        case STATE_UPDATE_PWM: // Estado de actualización del PWM
            if (state_entry_flag) 
            {
                pwm_ApplySteps(encoder_TakeSteps()); // Aplicamos los pasos del encoder al duty del modo activo
                state_entry_flag = 0; // Bajamos la bandera 
            }

            current_state = STATE_IDLE; // Volvemos al estado de reposo
            state_entry_flag = 1; // Subimos la bandera hacia IDLE
            break;

        case STATE_UPDATE_FREQ: // Estado de actualización de frecuencia
            if (state_entry_flag)
            {
                freq_Update(); // Actualizamos la frecuencia medida a partir del periodo capturado
                state_entry_flag = 0; // Bajamos la
                //  bandera
            }
            current_state = STATE_IDLE; // Volvemos al estado de reposo
            state_entry_flag = 1; // Subimos la bandera hacia IDLE
            break;

        case STATE_BLINK: // Estado de parpadeo del LED
            if (state_entry_flag)
            {
                led_Toggle(); // Conmutamos el estado del LED (PH1)
                state_entry_flag = 0; // Bajamos la bandera
            }

            current_state = STATE_IDLE; // Volvemos al estado de reposo
            state_entry_flag = 1; // Subimos la bandera hacia IDLE
            break;

        case STATE_ERROR:   // Estado de error: se recupera hacia IDLE
            if (state_entry_flag)
            {
                state_entry_flag = 0; // Bajamos la bandera
            }

            current_state = STATE_IDLE; // Volvemos al estado de reposo
            state_entry_flag = 1; // Subimos la bandera hacia IDLE
            break;

        default:
            current_state = STATE_IDLE; 
            state_entry_flag = 1;
            break;
        }
    }
}


/*============ Funciones driver de la FSM (el COMO) ============ */

/* Convierte las banderas de las ISR y el encoder en UN evento, con la prioridad: flanco del boton > fin del debounce > encoder > captura > blinky */
Event_t get_next_event(void)
{

    if (button_edge_flag == 1) // Bandera del primer flanco del boton SW del encoder
    {
        button_edge_flag = 0; // Bajamos la bandera
        return EV_BUTTON_EDGE; // Retornamos el evento de primer flanco del boton
    }

    if (debounce_flag == 1) // Bandera de fin de la ventana de debounce (TIM4)
    {
        debounce_flag = 0; // Bajamos la bandera
        return EV_DEBOUNCE_DONE; // Retornamos el evento de fin de debounce
    }

    
    int32_t counts = (int32_t) TIM5->CNT; //Con signo: al girar en sentido antihorario desde 0, CNT pasa a 0xFFFFFFFF, que es -1 

    if ((counts >= ENCODER_COUNTS_STEP) || (counts <= -ENCODER_COUNTS_STEP)) // 4 cuentas = 1 paso mecánico (modo encoder 3). Hay al menos un paso completo, CW o CCW
    {
        return EV_ENCODER_CHANGED; // Retornamos el evento de cambio en el encoder
    }

    if (capture_flag == 1) // Bandera de captura (TIM2)
    {
        capture_flag = 0;
        return EV_CAPTURE_READY; // Retornamos el evento de captura lista
    }

    if (blink_flag == 1) // Bandera del Blinky (TIM11) 
    {
        blink_flag = 0;
        return EV_BLINK_TICK; // Retornamos el evento de parpadeo del Blinky
    }

    return EV_NONE;
}

// Devuelve los pasos completos acumulados (+ horario, - antihorario) y los descuenta del contador TIM5. 
int32_t encoder_TakeSteps(void)
{
    int32_t counts;
    int32_t steps;

    counts = (int32_t) TIM5->CNT; // Guardamos el valor actual del contador del encoder (con signo)
    steps = counts / ENCODER_COUNTS_STEP; // Calculamos los pasos completos (4 cuentas = 1 paso mecánico del encoder)

    TIM5->CNT -= (uint32_t) (steps * ENCODER_COUNTS_STEP); // Descontamos los pasos completos del contador del encoder para no perder las cuentas que sobran (si CNT vale 6, se devuelve 1 paso y CNT queda en 2)

    return steps;
}

/*
  Aplica los pasos al duty del modo activo (edit_mode) y escribe los CCR de TIM3.
    MODE_RED / GREEN / BLUE : cambia SOLO el duty de ese color, los demas conservan su valor.
    MODE_RGB                : suma el mismo cambio a los tres; cada uno se satura por separado, asi que al llegar a un tope pierden la diferencia entre ellos.
    MODE_DAC                : cambia el duty del cuarto canal.

  Los que no cambiaron reciben el mismo valor que ya tenian, ya que se escriben los 4 CCR siempre.
 */
void pwm_ApplySteps(int32_t steps)
{

    int32_t delta = steps * PWM_DUTY_STEP; // Calculamos el cambio en el duty basado en los pasos del encoder y el paso de duty definido

    switch (edit_mode)
    {
    case MODE_RED: // Modo para editar el Duty del color Rojo
        duty_red = pwm_Saturate(duty_red + delta); // Calculamos el nuevo duty del color rojo y revisamos que no exceda los límites
        break;

    case MODE_GREEN: // Modo para editar el Duty del color Verde
        duty_green = pwm_Saturate(duty_green + delta); // Calculamos el nuevo duty del color verde y revisamos que no exceda los límites
        break;

    case MODE_BLUE: // Modo para editar el Duty del color Azul
        duty_blue = pwm_Saturate(duty_blue + delta); // Calculamos el nuevo duty del color azul y revisamos que no exceda los límites
        break;

    case MODE_RGB: // Modo para editar el Duty de los tres colores (Rojo, Verde y Azul) al mismo tiempo
        duty_red = pwm_Saturate(duty_red + delta);
        duty_green = pwm_Saturate(duty_green + delta);
        duty_blue = pwm_Saturate(duty_blue + delta);
        break;

    case MODE_DAC: // Modo para editar el Duty del canal DAC
        duty_dac = pwm_Saturate(duty_dac + delta); // Calculamos el nuevo duty del canal DAC, revisando que no exceda los límites
        break;

    default:
        break;
    }

    /* Los Nuevos valores de CCR en TIM3, asignando el valor correspondiente a cada uno (de acuerdo al canal) */
    TIM3->CCR1 = (uint32_t) duty_red; // Asignamos el nuevo duty del color rojo al canal 1 de TIM3
    TIM3->CCR2 = (uint32_t) duty_green; // Asignamos el nuevo duty del color verde al canal 2 de TIM3
    TIM3->CCR3 = (uint32_t) duty_blue; // Asignamos el nuevo duty del color azul al canal 3 de TIM3
    TIM3->CCR4 = (uint32_t) duty_dac; // Asignamos el nuevo duty del canal DAC al canal 4 de TIM3
}

//  Convierte el periodo capturado (en ticks) en frecuencia y se guarda en freq_hz
void freq_Update(void)
{

    uint32_t period = capture_period; // Leemos el periodo capturado por la ISR de TIM2 (con la resta de dos capturas)
    if (period != 0) // Si el periodo es diferente de cero, calculamos la frecuencia
    {
        freq_hz = CAPTURE_TICK_HZ / period; // Frecuencia = Constante de ticks / Periodo
    }
}

// Se limita el duty entre 0 y PWM_DUTY_MAX , para evitar valores fuera de rango 
static int32_t pwm_Saturate(int32_t duty)
{
    if (duty < 0) // Si el duty es menor que cero, lo limitamos a cero
    {
        return 0; // Minimo para el Duty
    }
    if (duty > PWM_DUTY_MAX) // Si el duty es mayor que el máximo permitido, lo limitamos a PWM_DUTY_MAX
    {
        return PWM_DUTY_MAX; // Maximo para el Duty
    }
    return duty; // Retornamos el duty sin cambios si está dentro del rango permitido
}

// Cambia el estado del LED PH1
void led_Toggle(void)
{
    GPIOH->ODR ^= GPIO_ODR_OD1; // XOR con 1 para conmutar el estado del Blinky
}

// Inicia el debounce del boton SW
void button_StartDebounce(void)
{
    EXTI->IMR &= ~EXTI_IMR_MR0; // Enmascaramos la linea 0 (EXTI0)
    EXTI->PR = EXTI_PR_PR0; // Se baja el pendiente escribiendo 1 en el bit
    TIM4->CNT = 0; // Reiniciamos el contador de TIM4 a cero
    TIM4->CR1 |= TIM_CR1_CEN; // Arranca la ventana de debounce
}

//  Termina la ventana de debounce
void button_EndDebounce(void)
{
    EXTI->PR = EXTI_PR_PR0; // Bajamos el pendiente de los rebotes, si no, EXTI0 dispararia otra interrupcion de inmediato
    EXTI->IMR |= EXTI_IMR_MR0; // Volvemos a habilitar la linea EXTI0
}

// Devuelve 1 si el boton esta presionado (PC0 en bajo, tiene pull-up) y 0 si no
uint8_t button_IsPressed(void)
{
    return ((GPIOC->IDR & GPIO_IDR_ID0) == 0) ? 1 : 0;
}


/*============ Funciones de inicialización del hardware ============ */

void init_gpio(void)
{

    /* Habilitamos el reloj del puerto GPIOH, que es para el Blinky de la Tarjeta didáctica */
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOHEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN;

    /* Habilitamos el reloj del puerto GPIOA*/
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;


    /* Habilitamos el reloj del puerto GPIOC*/
    RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOCEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;


    /* PH1 Blinky en la Tarjeta didactica, porque sin ella usariamos el PA5*/
    GPIOH->MODER  &= ~GPIO_MODER_MODE1; // Limpiamos el registro MODER del pin 1 del puerto GPIOH para configurar el pin como salida.
    GPIOH->MODER  |= GPIO_MODER_MODER1_0; // Configuramos el pin 1 del puerto GPIOH como salida.
    GPIOH->OTYPER &= ~GPIO_OTYPER_OT1; // Configuramos como salida push-pull.
    GPIOH->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR1; // Configuramos el pin 1 del puerto GPIOH como salida de baja velocidad.
    GPIOH->PUPDR &= ~GPIO_PUPDR_PUPD1; //No pull up/down
    GPIOH->ODR &= ~GPIO_ODR_OD1; // Inicializamos el pin 1 del puerto GPIOH en bajo, es decir, apagado.


    /*
     *   Pin   Funcion            Modo MODER           AF    Registro AFR
     *   ----  -----------------  -------------------  ----  ------------
     *   PA0   TIM5_CH1 (CLK)     funcion alternativa  AF2   AFR[0]
     *   PA1   TIM5_CH2 (DT)      funcion alternativa  AF2   AFR[0]
     *   PA5   TIM2_CH1 (captura) funcion alternativa  AF1   AFR[0]
     *   PA6   TIM3_CH1 (rojo)    funcion alternativa  AF2   AFR[0]
     *   PA7   TIM3_CH2 (verde)   funcion alternativa  AF2   AFR[0]
     *   PC8   TIM3_CH3 (azul)    funcion alternativa  AF2   AFR[1] (pines 8 a 15)
     *   PC9   TIM3_CH4 (DAC)     funcion alternativa  AF2   AFR[1]
     *   PC0   SW del encoder     entrada              -     -
     * MODER usa 2 bits por pin (10 = funcion alternativa) y AFR usa 4 bits por pin.
     */

    /* Puerto A: PA0, PA1, PA5, PA6 y PA7 en funcion alternativa */
    GPIOA->MODER &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1 | GPIO_MODER_MODE5 | GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
    GPIOA->MODER |= (GPIO_MODER_MODE0_1 | GPIO_MODER_MODE1_1 | GPIO_MODER_MODE5_1 | GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1);

    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFRL0 | GPIO_AFRL_AFRL1 | GPIO_AFRL_AFRL5 | GPIO_AFRL_AFRL6 | GPIO_AFRL_AFRL7);
    GPIOA->AFR[0] |= (GPIO_AFRL_AFRL0_1 | GPIO_AFRL_AFRL1_1 | GPIO_AFRL_AFRL6_1 | GPIO_AFRL_AFRL7_1);   /* PA0, PA1, PA6, PA7: 0010 (AF2) */
    GPIOA->AFR[0] |= GPIO_AFRL_AFRL5_0;  /* PA5: 0001 (AF1) */

    /* Puerto C: PC8 y PC9 en funcion alternativa AF2 (TIM3 CH3 y CH4) */
    GPIOC->MODER &= ~(GPIO_MODER_MODE8 | GPIO_MODER_MODE9);
    GPIOC->MODER |= (GPIO_MODER_MODE8_1 | GPIO_MODER_MODE9_1);

    GPIOC->AFR[1] &= ~(GPIO_AFRH_AFRH0 | GPIO_AFRH_AFRH1);
    GPIOC->AFR[1] |= (GPIO_AFRH_AFRH0_1 | GPIO_AFRH_AFRH1_1);

    /* PC0: entrada para el boton SW */
    GPIOC->MODER &= ~GPIO_MODER_MODE0;
    GPIOC->PUPDR &= ~GPIO_PUPDR_PUPD0;
    GPIOC->PUPDR |= GPIO_PUPDR_PUPDR0_0; //01: pull-up, en reposo lee 1 y al presionar baja a 0
}

void init_timer_BLINKY(void)
{
    /* Se va a usar el TIM11 para el blinky */
    RCC->APB2ENR &= ~RCC_APB2ENR_TIM11EN; // Limpiamos el reloj del timer TIM11
    RCC->APB2ENR |= RCC_APB2ENR_TIM11EN; // Habilitamos el reloj del timer TIM11

    TIM11->PSC = 16000 - 1; // Prescaler para obtener una frecuencia de 1 kHz (16 MHz / 16000 = 1 kHz).
    TIM11->ARR = 250 - 1; // Auto-reload para obtener un periodo de 250 ms (1 kHz / 250 = 4 Hz).
    TIM11->CNT = 0; // Inicializamos el contador en 0

    TIM11->DIER |= TIM_DIER_UIE; // Habilitamos la interrupción del temporizador TIM11
    TIM11->CR1 &= ~TIM_CR1_DIR; // Direccion ascendente
    TIM11->CR1 |= TIM_CR1_ARPE; // Habilitamos el auto-reload para que el contador se reinicie automaticamente cuando llegue al valor del auto-reload.

    TIM11->SR &= ~TIM_SR_UIF; // Limpiamos la bandera de interrupción del temporizador TIM11

    /* Matriculamos la interrupción del temporizador TIM11 que comparte la línea de interrupción del NVIC con TIM1 TRG/COM. */
    NVIC_EnableIRQ(TIM1_TRG_COM_TIM11_IRQn);

    TIM11->CR1 |= TIM_CR1_CEN; // Iniciamos el temporizador, es lo ultimo que se habilita
}

/*  TIM3: 4 canales PWM (R, G, B, DAC), todos con la misma frecuencia. */
void init_timer3_pwm(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;     /* Reloj de TIM3 */

    TIM3->PSC = PWM_PSC; //Tick de 1 us
    TIM3->ARR = PWM_ARR;
    TIM3->CCR1 = 0; // Definimos el ancho del pulso Inicialmente en 0, el duty inicia en 0 para el canal rojo
    TIM3->CCR2 = 0; // Definimos el ancho del pulso Inicialmente en 0, el duty inicia en 0 para el canal verde
    TIM3->CCR3 = 0; // Definimos el ancho del pulso Inicialmente en 0, el duty inicia en 0 para el canal azul
    TIM3->CCR4 = 0; // Definimos el ancho del pulso Inicialmente en 0, el duty inicia en 0 para el canal DAC
    TIM3->CR1 |= TIM_CR1_ARPE;              /* ARR con preload */

    /* Canales 1 y 2 en CCMR1: PWM modo 1 y preload de CCR */
    TIM3->CCMR1 &= ~(TIM_CCMR1_OC1M | TIM_CCMR1_OC2M); // Limpiamos
    TIM3->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2); //110: PWM modo 1, pin esta en alto mientras CNT < CCRx. ( nota recordatiria: ese registro tiene 3 posiciones desde 0 hasta 2 [2:0] entonces cada posicion seria: _2 _1 _0 , luego 110 seria _2 OR _1.)
    TIM3->CCMR1 |= (TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE); //CCR1 con preload


    /* Canales 3 y 4 en CCMR2 */
    TIM3->CCMR2 &= ~(TIM_CCMR2_OC3M | TIM_CCMR2_OC4M);
    TIM3->CCMR2 |= (TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2 );
    TIM3->CCMR2 |= (TIM_CCMR2_OC3PE | TIM_CCMR2_OC4PE);

    /* Habilita la salida de los 4 canales al pin */
    TIM3->CCER |= (TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E);

    TIM3->EGR |= TIM_EGR_UG;                /* Fuerza una actualización para cargar PSC, ARR y CCR antes de arrancar*/
    TIM3->CR1 |= TIM_CR1_CEN;               /* Arranca el contador, siempre de ultimo */
}

/* TIM5 (32 bits) en modo encoder 3, CLK en CH1 (PA0) y DT en CH2 (PA1) */
void init_tim5_encoder(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;     /* Reloj de TIM5 */

    TIM5->ARR = 0xFFFFFFFF; // Valor maximo del contador de 32 bits, para que no se desborde facilmente.
    TIM5->CCMR1 |= (TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0);   /* 01: CC1 y CC2 mapeados a TI1 y TI2 */
    TIM5->CCMR1 |= (TIM_CCMR1_IC1F | TIM_CCMR1_IC2F);       /* Filtro máximo: rechaza el rebote del encoder */
    TIM5->CCER |= (TIM_CCER_CC1E | TIM_CCER_CC2E);          /* Habilita la captura de los dos canales  CH1 y CH2 */

    TIM5->SMCR |= (TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1);        /* 011: modo encoder 3, ya que cuenta todos los flancos del encoder de TI1 y TI2 (Los 4 estados 01, 10, 00, 11 )*/
    TIM5->CR1 |= TIM_CR1_CEN;                               /* Arranca el contador */
}

/* TIM2 (32 bits) en captura de flanco de subida por CH1 (PA5), con interrupcion en cada captura. */
void init_tim2_capture(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;     /* Reloj de TIM2 */

    TIM2->PSC = CAPTURE_PSC; // Tick de 62.5 ns (16 MHz)
    TIM2->ARR = 0xFFFFFFFF; // Valor maximo del contador de 32 bits, para que no se desborde facilmente.

    TIM2->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_IC1F);
    TIM2->CCMR1 |= TIM_CCMR1_CC1S_0;                        // 01: CC1 captura TI1 
    TIM2->CCMR1 |= (TIM_CCMR1_IC1F_0 | TIM_CCMR1_IC1F_1);   // Filtro 0011 (N = 8 muestras a 16 MHz)

    TIM2->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);        // 00: flanco de subida
    TIM2->CCER |= TIM_CCER_CC1E;                            // Habilita la captura

    TIM2->DIER |= TIM_DIER_CC1IE; // Interrupcion por captura (primer interruptor)
    NVIC_EnableIRQ(TIM2_IRQn); // Matriculamos la interrupcion de TIM2 en el NVIC

    TIM2->CR1 |= TIM_CR1_CEN;               // Arranca el contador, siempre de ultimo 
}

/*  TIM4 (Timer_E): ventana de debounce del boton SW (PC0) de DEBOUNCE_MS milisegundos. */
void init_tim4_debounce(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN; // Reloj de TIM4

    TIM4->PSC = 16000 - 1; // 16 MHz / 16000 = 1 kHz (tick de 1 ms)
    TIM4->ARR = DEBOUNCE_MS - 1; // La ventana dura DEBOUNCE_MS ticks
    TIM4->CR1 |= TIM_CR1_OPM; // One-pulse: se detiene al terminar la cuenta

    TIM4->EGR |= TIM_EGR_UG; // Fuerza una actualizacion para cargar PSC y ARR (osino la primera ventana usaria PSC = 0)
    TIM4->SR &= ~TIM_SR_UIF; // La actualizacion forzada sube UIF, se baja antes de habilitar la interrupcion

    TIM4->DIER |= TIM_DIER_UIE; // Interrupcion por vencimiento de la ventana 
    NVIC_EnableIRQ(TIM4_IRQn); // Matriculamos la interrupcion de TIM4 en el NVIC
}

/* EXTI0 para PC0 (SW del encoder) */
void exti0_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;   // Reloj de SYSCFG: el multiplexor de EXTI vive ahi 

    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;  // Limpiamos la linea EXTI0
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PC;   // Pin 0 del puerto C (PC0 -> EXTI0)

    EXTI->FTSR |= EXTI_FTSR_TR0;  // Disparo por flanco de bajada (el boton entrega nivel alto en reposo y pasa a bajo al presionarlo).
    EXTI->IMR |= EXTI_IMR_MR0; // Habilitamos la mascara de interrupcion de la linea EXTI0 

    NVIC_EnableIRQ(EXTI0_IRQn); //Habilita la interrupción en el NVIC 
}


/* ============ Funciones ISR (registran el evento y regresan al IDLE) ============ */

void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
    if (TIM11->SR & TIM_SR_UIF) // Verificamos si la bandera esta subida.
    {
        TIM11->SR &= ~TIM_SR_UIF; // Bajamos la bandera, limpiando el registro.
        blink_flag = 1; //La FSM lo convierte en EV_BLINK_TICK
    }
}

/* Boton SW (PC0): primer flanco de bajada. Solo deja la bandera, la FSM decide que hacer (STATE_DEBOUNCE_START) */
void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR0) // Verificamos si la bandera esta subida.
    {
        EXTI->PR = EXTI_PR_PR0; // Se baja escribiendo 1 en el bit, no 0 como TIM->SR.
        button_edge_flag = 1; // La FSM lo convierte en EV_BUTTON_EDGE
    }
}

/* Vence la ventana de debounce de TIM4 (Timer_E). Solo deja la bandera, la FSM decide que hacer (STATE_DEBOUNCE_CHECK) */
void TIM4_IRQHandler(void)
{
    if (TIM4->SR & TIM_SR_UIF) // Verificamos si la bandera esta subida.
    {
        TIM4->SR &= ~TIM_SR_UIF; // Bajamos la bandera, limpiando el registro.
        debounce_flag = 1; // La FSM lo convierte en EV_DEBOUNCE_DONE
    }
}

/* Captura de TIM2 y se envia el periodo entre los dos ultimos flancos  */
void TIM2_IRQHandler(void)
{
    uint32_t capture_now;

    if (TIM2->SR & TIM_SR_CC1IF) // Verificamos si la bandera esta subida.
    {
        capture_now = TIM2->CCR1;  // Lee el valor del registro de captura, que es el tiempo del flanco actual.
        capture_period = capture_now - capture_last; // Calcula el periodo entre el flanco actual y el anterior. 
        capture_last = capture_now; // Guarda el tiempo del flanco actual para la proxima interrupcion.
        capture_flag = 1;       // La FSM lo convierte en EV_CAPTURE_READY 
    }
}
