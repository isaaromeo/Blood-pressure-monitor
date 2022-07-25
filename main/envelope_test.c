/*
 * envelope_test.c
 *
 *  Created on: 20 sept. 2021
 *      Author: romeofernandoromeo
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <envelope.h>
#include <spline_interpolation.h>
#include <fifo_queue.h>

float y_test[22]={1.2, 0.8, 1, 1.3, 1.45, 1.5, 1.4, 1.4, 1.2, 0.9, 0.55, 0.8, 1, 1.23, 1.75, 1.5, 1.4, 1.33, 1.2, 0.9, 0.55, 2.3 };
int x_test[22];
int minD = 30;
int n = length(y_test);
struct fifo_queue oscil_test;
struct envelope env_test;

void app_main(void){
//llenamos el aray de indices x
for(int i=0; i<=length(y_test); i++){

	x_test[i] = i;

	}
//inicializamos los structs de las oscilaciones y el envelope

init_fifo_queue(&oscil_test, 22);
init_envelope(&env_test, minD, n);

//Buscamos los maximos
envPeaks(&env_test, &oscil_test);

//Interpolamos los maximos
int ni = n*9;
float xi[ni];
float yi[ni];
SPL(n, env_test->x, env_test->y, ni, xi, yi);

//printeamos para ver los resultados

for(int i=0; i<=length(y_test); i++){

	printf("Oscil envelope %.4f \r\n", yi[i]);

	}


}
///esfld0921  Beili001test otwoo09254 306797975
