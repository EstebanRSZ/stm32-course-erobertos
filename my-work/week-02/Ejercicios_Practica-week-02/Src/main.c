/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Esteban Roberto Saiz , erobertos@unal.edu.co
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
void ejercicio_and(void);

int main(void)
{
    ejercicio_and();
    while(1)
    {
        // Main loop code here
    }
}

/* Ejercicio 2.1 Operador AND: la máscara que revela */

void ejercicio_and()
{
    uint8_t value = 0b10110101;
    uint8_t mask =  0b00001111;
    uint8_t result = value & mask; // El resultado obtenido es 00000101 (5 en decimal)
    // Y repondiendo a la pregunta: ¿qué máscara usarías para extraer solo los 4 bits superiores (bits 7 a 4)?
    // La respuesta es: 0b11110000 (240 en decimal)
}

/* Ejercicio 2.2 Operador OR: la máscara que establece */
void ejercicio_or()
{
    uint8_t value = 0b10100000;
    uint8_t mask =  0b00000101;
    uint8_t result = value | mask; // El resultado obtenido es 10100101 (165 en decimal)
}


/* Ejercicio 2.3  Operador NOT: el complemento */
void ejercicio_not()
{
    uint8_t a = 0b00001111;
    uint8_t result1 = ~a;

    uint8_t b = 0b10100101;
    uint8_t result2 = ~b;

    /*  Ahora vamos a declarar uint8_t value = 0b11111111; y luego limpiar los bits 0 y 1
    sin cambiar ningún otro bit. Usando el patrón &= ~()*/
    uint8_t value = 0b11111111;
    uint8_t mask = 0b00000011; // Máscara para limpiar los bits 0 y 1
    value &= ~mask; // Limpia los bits 0 y 1, el resultado es 11111100 (252 en decimal)
}

/* Ejrecicio 2.4 Operador XOR: el toggle */
void ejercicio_xor()
{
    uint8_t value = 0b10110011;
    uint8_t mask = 0b00001111;
    uint8_t result1 = value ^ mask;
    uint8_t result2 = result1 ^ mask;
    
}