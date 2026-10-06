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

#include <stdint.h>
void operadores_aritmeticos(void);
void desbordamiento(void);
void desplazmiento_mult_div(void);
void evaluacion_booleana(void);
void bucle_contador_plus(void);
void bucle_contador_two(void);
void bucle_contador_negative(void);
void bucle_acumulador_simple(void);
void bucle_do_while(void);
void switch_case_decision_tree(void);

int main(void)
{

    //operadores_aritmeticos();
    //desbordamiento();
    //bucle_contador_negative();
    //evaluacion_booleana();
    //desplazmiento_mult_div();
    //bucle_acumulador_simple();
    switch_case_decision_tree();
    
    while (1)
    {
        // Main loop code here
    }


}

/* Ejercicio 1.1 - Operadores Aritméticos */
void operadores_aritmeticos(void)
{
    uint8_t a  = 17;
    uint8_t b  = 5;
    uint8_t div_result = a / b; // Integer division, daria 3 debido a que 17/5 = 3.4, pero como es integer division, se descarta la parte decimal.
    uint8_t mod_result = a % b; // Modulus operation, daria 2 debido a que el modulo es el residuo de la division, 17/5 = 3 con residuo 2.
    uint8_t mult_result = a * b; // Multiplication operation, daria 85 debido a que 17*5 = 85.
}

/* Ejercicio 1.2 - Desbordamiento */
void desbordamiento(void)
{
    uint8_t a = 200; 
    uint8_t b = 100;
    uint8_t result = a + b; // Hay un desbordamiento porque daria 300, pero el rango de uint8_t es de 0 a 255, entonces el resultado real seria 44 (300 - 256 = 44).
}

/* Ejercicio 1.3 - Desplazamiento, Multiplicación y División */
void desplazmiento_mult_div(void)
{
    uint8_t val = 3; // 00000011 en binario
    uint8_t left1 = val << 1; // Desplazamiento a la izquierda por 1 bit, daria 6 (00000110 en binario)
    uint8_t left2 = val << 2; // Desplazamiento a la izquierda por 2 bits, daria 12 (00001100 en binario)
    uint8_t left3 = val << 3; // Desplazamiento a la izquierda por 3 bits, daria 24 (00011000 en binario)
    uint8_t right1 = val >> 1; // Desplazamiento a la derecha por 1 bit, daria 1 (00000001 en binario)
}

/* Ejercicio 1.4 - Evaluación Booleana */
void evaluacion_booleana(void)
{

    /* Escribimos un programa que declare tres variables con diferentes valores, luego use sentencias if e if-else para establecer una variable result
basándose en condiciones. La lógica debe probar al menos un valor distinto de cero como condición, un valor cero como condición y una
comparación de igualdad. Coloca un breakpoint después de cada bloque if y observa cómo cambia result en el depurador mientras recorres
el código. */

    uint8_t a = 5; // Variable con valor distinto de cero
    uint8_t b = 0; // Variable con valor cero
    uint8_t c = 15; // Variable con valor igual a 15

    uint8_t result = 0; // Variable para almacenar el resultado basado en las condiciones

    if (a != 0) {
        // Si a es distinto de cero, asignamos 1 a result

        result = 1;
    } 
    
    else if (b == 0) {
        // Si b es igual a cero, asignamos 2 a result
        result = 2;

    } 
    
    else if (c == 15) {
        // Si c es igual a 15, asignamos 3 a result
        result = 3;

    } 
    
    else {
        // Si ninguna condición anterior se cumple, asignamos 4 a result
        result = 4;

    }
}

/*Ejercicio 1.5 - El Bucle for como contador*/

void bucle_contador_plus(void)
{
    uint8_t i;
    uint8_t contador = 0; // Variable para contar el número de iteraciones

    for ( i = 0; i < 9; i++) // Contador que va de 0 a 8, es decir, 9 iteraciones en total
    {
        contador++;
    }

}

void bucle_contador_two(void)
{
    uint8_t i;
    uint8_t contador = 0; // Variable para contar el número de iteraciones

    for ( i = 0; i < 9; i += 2) // Contador que va de 0 a 8,con un paso de 2, es decir, 5 iteraciones en total (0, 2, 4, 6, 8)
    {
        contador ++; // Incrementamos el contador en 2 en cada iteración
    }

}

void bucle_contador_negative(void)
{
    uint8_t i;
    uint8_t contador = 0; // Variable para contar el número de iteraciones

    for (i = 10; i > 0; i--) // Contador que va de 10 a 1, es decir, 10 iteraciones en total
    {
        contador++; // Incrementamos el contador en 1 en cada iteración
    }

}


/* Ejercicio 1.6 - El Bucle while como acumulador simple*/

void bucle_acumulador_simple(void)
{
    uint16_t i = 0;
    uint16_t acumulador = 0; // Variable para acumular el valor de i

    while (i < 100) // Mientras i sea menor que 100, como las variables son uint16_t, el bucle se detendrá cuando i sea igual a 100, ya que el rango de uint16_t es de 0 a 65535. Mientas que si hubiese sido uint_8
    {
        acumulador += i; // Acumulamos el valor de i en la variable acumulador, y en este caso finalizari en 4950, ya que la suma de los primeros 99 números naturales es 4950.
        i++; // Incrementamos i en 1, y en este caso finalizari en 100, ya que el bucle se detiene cuando i es igual a 100.
    }

}

/*Ejercicio 1.7 - El bucle do-while*/
void bucle_do_while(void)
{
    uint16_t i = 0;
    uint16_t acumulador = 0; // Variable para acumular el valor de i

    // Primero probamos con un bucle while que no se ejecuta, para ver que el acumulador queda en 0.
    while (0) // Esta condición es falsa, por lo que el bucle while no se ejecutará, y el valor de acumulador será 0.
    {
        acumulador += i; // Acumulamos el valor de i en la variable acumulador
        i++; // Incrementamos i en 1
    }

    // Ahora probamos con un bucle do-while que se ejecuta al menos una vez, para ver que el acumulador queda en 0.
    do
    {
        acumulador += 1; // Acumulamos el valor de i en la variable acumulador
        i++; // Incrementamos i en 1
    } while (0); // Esta condición es falsa, por lo que el bucle do-while se ejecutará al menos una vez, y el valor de acumulador será 1.

    // La diferencia fundamental entre ambos bucles es que el bucle while evalúa la condición antes de ejecutar el bloque de código, mientras que el bucle do-while ejecuta el bloque de código al menos una vez antes de evaluar la condición. Por lo tanto, en este caso, el bucle do-while permite que el acumulador se incremente en 1, mientras que el bucle while no lo hace.
}



/* Ejercicio 1.8 - El switch-case como arbol de decisiones */
void switch_case_decision_tree(void)
{
    uint8_t opcion = 1;
    uint8_t acumulador = 0; // Variable para almacenar el resultado basado en la opción seleccionada

    switch (opcion)
    {
        case 1:
            // Acción para la opción 1
            acumulador = 1;
            // Como no tiene un break, entonces va a continuar con el case 2.
        case 2:
            // Acción para la opción 2
            acumulador = 2;
            break;
        case 3:
            // Acción para la opción 3
            acumulador = 3;
            break;
        case 4:
            // Acción para la opción 4
            acumulador = 44;
            break;

        default:
            // Acción por defecto
            acumulador = 0;
            break;
    }
}