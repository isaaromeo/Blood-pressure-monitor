/*
 * findPeaks.c
 *
 *  Created on: 5 oct. 2021
 *      Author: romeofernandoromeo
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "fifo_queue.h"
#include "findPeaks.h"

void init_envelope(struct envelope *env, int d, int nelems){

	env->MAXSIZE = nelems;
	env->y = malloc(nelems*sizeof(float));
	env->minD=d;
	env->x = malloc(nelems*sizeof(float));
	env->MAP = 0.;
	env->MAP_idx = 0;
	env->As= 0.;
	env->As_idx= 0;
	env->Ad= 0.;
	env->Ad_idx= 0;
	//env->iPk = malloc(nelems*sizeof(float));

}
void free_envelope(struct envelope *env){
	free(env->y);
	free(env->x);
	env->MAXSIZE=0;
	env->minD=0;
	env->MAP = 0.;
	env->MAP_idx = 0;
	env->As= 0.;
	env->As_idx= 0;
	env->Ad= 0.;
	env->Ad_idx= 0;


}
void findPeaks(struct envelope *env, struct fifo_queue *oscil_norm){

	float *yTemp;
	int oscil_size = oscil_norm->size;
	printf("oscil_size= %d \n", oscil_size );
	yTemp =  malloc((oscil_size +2)*sizeof(float));
	for(int i=0; i<(oscil_size +2); i++){

		if(i==0){
			yTemp[i]=0;}
		else if(i==oscil_size +1){
			yTemp[i] = 0;
			}
		else {
			yTemp[i] = oscil_norm->arr_norm[i-1];} ////////arr_norm
			}


	//Hacemos resta de los valores adyacentes a lo largo del array
	//y sacamos el signo de cada valor
	float *diff;
	signed int *s;
	diff = malloc((oscil_size+2)*sizeof(float));
	s = malloc((oscil_size+2)*sizeof(int));
	printf("g \n" );
	for(int i=0; i<(oscil_size+2); i++){
		if(i>0){
			diff[i-1] = yTemp[i]-yTemp[i-1];
		}
	}

	for(int i=0; i<(oscil_size+2); i++){
		if(diff[i]>0){
			s[i] = 1;
		}
		else if(diff[i]<0){
			s[i] = -1;
		}
		else s[i] = 0;
	}


	//Buscamos los maximos locales (su localizacio, indice)
	//para ello hacemos la resta de los valores adyacentes a lo largo de s
	//y buscamos donde sea menor de 0
	signed int *diffS;
	diffS = malloc((oscil_size+2)*sizeof(int));

	struct fifo_queue iMax;
	init_fifo_queue( &iMax, (oscil_size+2));

	for(int i=0; i<(oscil_size+2); i++){
		if(i>0){
			diffS[i-1] = s[i]-s[i-1];
		}
	}

	for(int i=0; i<(oscil_size+2); i++){
		if(diffS[i]<0){
			enqueue(&iMax, i);
			//printf("iMax: %d \r\n", i);
		}
	}
	
	struct fifo_queue iPk;
	init_fifo_queue( &iPk, (oscil_norm->size+2));
	int size_iMax = iMax.size;
	for(int i=0; i<=size_iMax; i++){
		enqueue(&iPk, i-1);
	}


	//findPeaksSeparatedByMoreThanMinPeakDistance
	//guardamos peaks y locs en dos variables
	int size_iMax = iMax.size;
	float pks[size_iMax];
	float locs[size_iMax];
	//locs=iPk;
	printf("k \n" );
	for(int i=0; i<size_iMax; i++){
			pks[i] = oscil_norm->arr_norm[(int)iMax.arr[i]];/// //////////////arr_norm
			locs[i] = iMax.arr[i];
		}

	//ordenamos los Peaks de mayor a menor y guardamos
	//los idx ordenados tmbn en otro array
	printf("size_iMax= %d \n", size_iMax );

	struct sort_queue
	{
		float value;
		int idx;
	};

	struct sort_queue sortIdx[size_iMax];

	for (int i = 0; i < size_iMax; i++)
		{
			sortIdx[i].value = pks[i];
			sortIdx[i].idx = (int)locs[i];
		}

	int comp(const void *a, const void *b)
	{
		struct sort_queue *a1 = (struct sort_queue *)a;
		struct sort_queue *a2 = (struct sort_queue *)b;
		if ((*a1).value > (*a2).value)
			return -1;
		else if ((*a1).value < (*a2).value)
			return 1;
		else
			return 0;
	}

	 qsort(sortIdx, size_iMax, sizeof(sortIdx[0]), comp);//decending function

	 //una vez tenemos el array de idx ordenado obtenemos el
	 //array de x indexando sortIdx

	 for(int i=0; i<size_iMax; i++){
		 locs[i] = sortIdx[i].idx;
		 //printf("sort_locs: %f \n ", locs[i]);
	 }
	 printf("s \n" );
	 int idelete[size_iMax];
	 for(int i=0; i<size_iMax; i++){
			idelete[i]=0;
		}

	//If the peak is not in the neighborhood of a larger peak, find
	//secondary peaks to eliminate.
	 int masc1[size_iMax];
	 int masc2[size_iMax];
	 printf("t \n" );
	 for(int i=0; i<size_iMax; i++){
		 if(!idelete[i]){

		   for(int k=0; k<size_iMax; k++){
			if(locs[k]>=(locs[i]-env->minD)){
				masc1[k]=1;
			} else masc1[k]=0;

			if(locs[k]<=(locs[i]+env->minD)){
				masc2[k]=1;
			} else masc2[k]=0;

		 }

		 for(int j=0; j<=size_iMax; j++){
			 idelete[j] = idelete[j] | (masc1[j] & masc2[j]);
		 }
		 idelete[i] = 0; //keep current peak

	   }
	 }

	 //Para cribar los maximos solo nos quedaremos con los maximos
	 //en los que el valor en el indice correspondiente en el array idelete
	 //sea 0. -> Ej: sortValue=[5, 3, 2 ,1] sortIdx=[2, 3, 1, 0] idelete=[0, 0, 1, 0]
	 //nos quedaremos con los maximos situados en 2 3 y 0

	 //Para ello crearemos otro struct sort con los valores definitivos

	 //Debemos contar cuantos maximos quitamos para saber el size del nuevo
	 //struct de valores
	 int cnt=0;
	 for(int i=0; i<size_iMax; i++){
		 if(idelete[i]==1){
			cnt++;
		 }
		}

	 int size_def = size_iMax-cnt;
	 struct sort_queue def[size_def];
	 cnt=0;

	 for(int i=0; i<size_iMax; i++){
		 if(idelete[i]==0){
			 //printf("definitivo idx: %d \n ", size_def);
			 def[cnt].value = sortIdx[i].value;
			 def[cnt].idx = sortIdx[i].idx;
			 cnt++;
			 //printf("definitivo value: %f \n ", sortIdx[i].value);
			 //printf("definitivo idx: %d \n ", sortIdx[i].idx);
		 }
		}

	 cnt=0;

	 //Una vez tenemos seleccionados los maximos los volvemos a reordenar
	 //en orden cronologico
	 int comp2(const void *a, const void *b)
	 {
		 struct sort_queue *a1 = (struct sort_queue *)a;
		 struct sort_queue *a2 = (struct sort_queue *)b;
		 if ((*a1).idx > (*a2).idx)
			 return 1;
		 else if ((*a1).idx < (*a2).idx)
			 return -1;
		 else
			 return 0;
	 }
	 
	 qsort(def, size_def, sizeof(def[0]), comp2);

	 printf("size_def= %d \n",size_def );
	 //Una vez lo tenemos reordenado guardamos los idx
	 //hacemos bookend con 0 a los arrays de valores e indices finales pra interpolar bien
	 //int idx[iMax->size];
	 for(int i=0; i<size_def+2; i++){
		 //printf("entra for \n" );
		 //idx[i]=sortIdx[i].value;
		 if((i==0) | (i==(size_def+1))){
			 //printf("i0= %d \n",i );
			 env->y[i]=0;
			 env->x[i]=0;
		 }
		 else {
		 //printf("ix= %d \n",i );
		 env->y[i]=def[i-1].value;
		 env->x[i]=def[i-1].idx;
		 }
		 //printf("y final: %f \n ", env_test.y[i]);
		 //printf("x final: %f \n ", env_test.x[i]);

			}


	float xi[oscil_size +2];
	float yi[oscil_size +2];
	printf("1 \n" );
	//Inicializamos el array de valores de x de la iterpolacion
	for(int i=0; i<oscil_size+2; i++){
		xi[i] = i;

	}

	//Algoritmo 2

	 printf("x \n" );
	for(int i=0; i<(size_def+1); i++){
		int x0 = env->x[i];
		int x1 = env->x[i+1];
		float y0 = env->y[i];
		float y1 = env->y[i+1];
		yi[x0]=y0;
		yi[x1]=y1;
		//printf("intervalo  %d  ", x0);
		//printf("-- %d \n", x1);

		for(int j=x0+1; j<x1; j++){
			int xp = xi[j];
			float yp = y0+ ((y1-y0)/(x1-x0))*(xp-x0);
			yi[xp]=yp;
			//printf("x= %d ", xp);
			//printf("yp= %f \n", yp);
		}


	}


	//Una vez tenemos la envelope hecha solo queda buscar el maximo que sera el MAP
	//Una vez tengamos el MAP las medidas de SYS y DIAS seran unos porcentaje fijos
	//del MAP(por arriba o por abajo)
	float max=0;
	int max_idx=0;
	//Para Buscar el MAP buscaremos a partir del indice 500 + o - mpara evitar los
	//maximos del principio
	for(int i=500; i<(oscil_size-100); i++){
		//printf("yi[i] %f \n", yi[i] );
		if((yi[i])>max){
			max=(yi[i]);
			max_idx=i;
		}
	}

	float MAP=max;
	printf("MAP %f \n", MAP );
	printf("MAP_idx %d \n", max_idx );
	float Ad=0.5*MAP;
	float As=0.75*MAP;
	//Buscamos el idx donde se encuentra As y Ad
	float d=5;
	float d_min=5;
	int x_min_sys=0;
	int x_min_dias=0;
	printf("As %f \n", As );
	for(int i=500; i<max_idx; i++){
		d=abs((yi[i])-abs(As));
		if(d<d_min){
			d_min=d;
			x_min_sys=i;
		}

	}
	printf("Ad %f \n", Ad );
	d=5;
	d_min=5;
	for(int i=max_idx; i<1500; i++){
		d=abs((yi[i])-abs(Ad));
		if(d<d_min){
			d_min=d;
			x_min_dias=i;
		}

	}
	env->MAP = MAP;
	env->As = As;
	env->Ad = Ad;
	env->MAP_idx = max_idx;
	env->As_idx = x_min_sys;
	env->Ad_idx = x_min_dias;

	printf("SYS_idx %d \n", env->As_idx );
	printf("DIAS_idx %d \n", env->Ad_idx );

	//Obtenemos los valores de presion dias y sys con los indices obtenidos
	//float press_sys=press_queue.arr[x_min_sys];
	//float press_dias=press_queue.arr[x_min_dias];

	//Vaciamos las colas
	//free_queue(&oscil_norm);
	free(yTemp);
	free(diff);
	free(s);
	free_queue(&iMax);
	//CHECKPOINT 7

	}

