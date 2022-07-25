/*
 * fifo-queue.c
 *
 *  Created on: 4 jul. 2021
 *      Author: romeofernandoromeo
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "fifo_queue.h"

void init_fifo_queue(struct fifo_queue *queue, int nelems){
	queue->MAXSIZE = nelems;
	queue->arr = malloc(nelems*sizeof(float));
	queue->arr_norm = malloc(nelems*sizeof(float));
	queue->front = -1;// first elem index
	queue->rear = -1; //actual length
	queue->size = 0; //total size
	queue->max = 0;
	queue->min = 0;
	//queue->arr_norm = malloc(nelems*sizeof(float));
}

void dequeue(struct fifo_queue *queue){
	if(queue->size<0)
	    {
	        printf("Queue is empty\n");
	    }
	    else
	    {
	    	queue->size--;
	    	queue->front++;
	    }

}

void enqueue(struct fifo_queue *queue, float value){

	if(queue->size < queue->MAXSIZE)
	    {
	        if(queue->size < 0)
	        {
	        	queue->arr[0] = value;
	        	queue->front = queue->rear = 0;
	        	queue->size = 1;
	        }
	        else if(queue->rear == queue->MAXSIZE-1)
	        {
	        	queue->arr[0] = value;
	        	queue->rear = 0;
	        	queue->size++;
	        }
	        else
	        {
	        	queue->arr[queue->rear+1] = value;
	        	queue->rear++;
	        	queue->size++;
	        }
	    }
	    else
	    {
	        printf("Queue is full\n");
	    }
}

float last_queue_value(struct fifo_queue *queue){
	return queue->arr[queue->rear];
}

void free_queue(struct fifo_queue *queue){
	free(queue->arr);
	free(queue->arr_norm);
	queue->front = queue->rear = 0;
	queue->size = 1;
}

int min_max_n(struct fifo_queue *queue)
{
    //int a[1000],i,n,min,max;
    double n = queue->size;
    int i=0;
    float min = 0;
    float max = 0;

    //printf("Enter size of the array : ");
    //scanf("%d",&n);

    //printf("Enter elements in array : ");
    //for(i=0; i<n; i++)
    //{
      //  scanf("%d",&a[i]);
    //}

    min=max=queue->arr[0];
    for(i=0; i<n; i++)
    {
         if(min>queue->arr[i]){
        	 min=queue->arr[i];
         }

		 if(max<queue->arr[i]){
			 max=queue->arr[i];
		 }

    }
    queue->max=max;
    queue->min=min;
    printf("minimum of array is : %f \n",min);
    printf("maximum of array is : %f \n",max);
    return n;



}

void norm(struct fifo_queue *queue)

{
	int N = queue->size;
	//1 sacamos la media
	float suma = 0;
	for(int i=0; i<N; i++){
		suma = suma + queue->arr[i];
		//printf("y: %f \n", env->y[i]);
		//printf("suma: %f \n", suma);
	}
	float mean = 0;
	mean = suma/(float)N;
	//float *normalized = (float*) malloc(n*sizeof(float));
	int i = 0;
	for(i=0; i<N; i++)
	{
		//queue->arr_norm[i] = ((queue->arr[i])-(queue->min))/((queue->max)-(queue->min));
		queue->arr_norm[i] = (queue->arr[i] - mean);
	}
	//return normalized;

}



