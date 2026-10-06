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

void prueba_variables(void);
void bit_a_bit(void);
void operador_bang(void);

int main(void)
{

    // prueba_variables();
    // bit_a_bit();
    operador_bang();

    while (1)
    {
        // Main loop code here
    }

}


void operador_bang(void)
{
    /*Vamos a probar el operador de negación (!) en C, donde esto lo que hace es 
    convertir un valor diferente de cero en cero, y un valor igual a cero en uno. 
    Esto se logra invirtiendo el valor lógico de la expresión. Por ejemplo, si tenemos una variable a con un valor de 5, 
    al aplicar el operador !a, obtendremos 0, ya que 5 es diferente de cero. Por otro lado, si tenemos una variable b con un valor de 0, 
    al aplicar el operador !b, obtendremos 1, ya que 0 es igual a cero.
    */
    uint8_t a = 5;
    uint8_t b = 0;
    uint8_t c = 255;
    uint8_t d = 0;

    uint8_t r1 = !a; // r1 será 0 porque a es diferente de 0
    uint8_t r2 = !b; // r2 será 1 porque b es igual a 0
    uint8_t r3 = !c; // r3 será 0 porque c es diferente de 0
    uint8_t r5 = !d; // r5 será 1 porque d es igual a 0

    uint8_t r4 = ~c; // r4 será 0 porque c es diferente de 0, pero ~c es la negación bit a bit de c, lo que significa que todos los bits de c se invierten. En este caso, c es 255 (11111111 en binario), por lo que ~c será 0 (00000000 en binario).
}


void bit_a_bit(void)
{
    uint8_t x = 0;
    x = 0x01;
    x = 0x02;
    x = 0x04;
    x = 0x08;
    x = 0x10;
    x = 0x20;
    x = 0x40;
    x = 0x80;

}


void prueba_variables(void)
{
    int8_t x = 127;
    x = x + 1; // Esto causa desbordamiento y el valor se ajusta a -128 debido a que 127 + 1 = 128, pero en complemento a dos, 128 representa -128

    uint8_t y = 255;
    y = y + 1; // Esto causa desbordamiento y el valor se ajusta a 0 debido a que 255 + 1 = 256, pero en complemento a dos, 256 representa 0


    uint8_t my_variable = 42;

    uint8_t dec = 65;
    uint8_t hex = 0x41;
    uint8_t bin = 0b01000001;


    uint8_t a = 255;
    uint16_t b= 256;
    uint32_t c =255;
    uint8_t d = 256; // Esto causa desbordamiento y el valor se ajusta a 0

    uint8_t e = 257;  // Esto causa desbordamiento y el valor se ajusta a 1 debido a que 257 mod 256 = 1


    uint8_t g = 200;
    int8_t h = 200; // Esto causa desbordamiento y el valor se ajusta a -56 debido a que 200 mod 256 = 200, pero en complemento a dos, 200 representa -56
    int8_t f = -1; // Esto representa el valor -1 en complemento a dos, es decir se mostra como 11111111 en binario, que es equivalente a 255 en decimal si se interpreta como un número sin signo.
    int8_t i = -2; // Esto representa el valor -2 en complemento a dos, es decir se mostra como 11111110 en binario, que es equivalente a 254 en decimal si se interpreta como un número sin signo.

}