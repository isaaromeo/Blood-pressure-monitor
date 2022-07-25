/*
 * findPeaks.h
 *
 *  Created on: 5 oct. 2021
 *      Author: romeofernandoromeo
 */

#ifndef MAIN_FINDPEAKS_H_
#define MAIN_FINDPEAKS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "fifo_queue.h"

struct envelope{

	int MAXSIZE;
	float *y;
	int minD;
	//float *iPk;
	float *x;
	float MAP;
	int MAP_idx;
	float As;
	int As_idx;
	float Ad;
	int Ad_idx;


};
void init_envelope(struct envelope *env, int d, int nelems);
void free_envelope(struct envelope *env);
void findPeaks(struct envelope *env, struct fifo_queue *oscil_norm);

#endif /* MAIN_FINDPEAKS_H_ */
