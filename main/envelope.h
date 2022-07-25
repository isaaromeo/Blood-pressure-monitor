/*
 * envelope.h
 *
 *  Created on: 18 sept. 2021
 *      Author: romeofernandoromeo
 */

#ifndef MAIN_ENVELOPE_H_
#define MAIN_ENVELOPE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fifo_queue.h>


struct envelope{

	int MAXSIZE;
	float *y;
	int minD;
	int *iPk;
	int *x;


};

void init_envelope(struct envelope *env, double d, int MAXSIZE);

void findPeaks(struct envelope *env, struct fifo_queue *oscil_norm);

#endif /* MAIN_ENVELOPE_H_ */
