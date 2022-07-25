/*
 * fifo_queue.h
 *
 *  Created on: 5 jul. 2021
 *      Author: romeofernandoromeo
 */

#ifndef MAIN_FIFO_QUEUE_H_
#define MAIN_FIFO_QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


struct fifo_queue{

	int MAXSIZE;
	float *arr;
	int front;
	int rear;
	int size;
	float max;
	float min;
	float *arr_norm;

};

void init_fifo_queue(struct fifo_queue *queue, int nelems);

void dequeue(struct fifo_queue *queue);

void enqueue(struct fifo_queue *queue, float value);

float last_queue_value(struct fifo_queue *queue);

void free_queue(struct fifo_queue *queue);

int min_max_n(struct fifo_queue *queue);

void norm(struct fifo_queue *queue);


#endif /* MAIN_FIFO_QUEUE_H_ */
