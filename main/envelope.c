/*
 * envelope.c
 *
 *  Created on: 18 sept. 2021
 *      Author: romeofernandoromeo
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <envelope.h>
#include <fifo_queue.h>

void init_envelope(struct envelope *env, int d, int nelems){
	env->MAXSIZE = nelems;
	env->y = malloc(nelems*sizeof(float));
	env->minD=d;
	env->x = malloc(nelems*sizeof(float));

}

void findPeaks(struct envelope *env, struct fifo_queue *oscil_norm){
	//creamos array del eje x
	int x[oscil_norm->size];
	for(int i=0; i<=length(x); i++){
		x[i]=i;
	}
	//creamos variable temporal y hacemos bookend con 0
	float *yTemp;
	yTemp=malloc((oscil_norm->size+2)*sizeof(float));
	int zero[1]={0};
	memcpy(yTemp, zero, 1*sizeof(float));
	memcpy(yTemp + (oscil_norm->size), oscil_norm->arr, (oscil_norm->size)*sizeof(float));
	memcpy(yTemp + 1, zero, 1*sizeof(float));

	//Creamos array de indices
	double iTemp[length(yTemp)];
	//Lo inicializamos
	for(int i=0; i<=length(yTemp); i++){
		iTemp[i]=i;
	}
	//buscar si existen pares de valores iguales y nos quedamos solo con el primero
	/*we might not need this part
	int *iNeq;
	iNeq = malloc((oscil_norm->size+2)*sizeof(float));
	for(int i=0; i<=length(yTemp); i++){
		if(i>0){
			if(yTemp(i) != yTemp(i-1)){

				iNeq[i-1]=(i-1);
				iNeq[i]=i;

			}
			else iNeq[i-1]=i-1;
		}
	}
	*/

	//Hacemos resta de los valores adyacentes a lo largo del array
	//y sacamos el signo de cada valor
	float *diff;
	signed int *s;
	diff = malloc((oscil_norm->size+2)*sizeof(float));
	s = malloc((oscil_norm->size+2)*sizeof(int));

	for(int i=0; i<=length(diff); i++){
		if(i>0){
			diff[i-1] = iTemp(i-1)-iTemp(i);
		}
	}

	for(int i=0; i<=length(s); i++){
		if(diff[i]>0){
			s[i] = 1;
		}
		else if(s[i]<0){
			s[i] = -1;
		}
		else s[i] = 0;
	}

	//Buscamos los maximos locales (su localizacio, indice)
	//para ello hacemos la resta de los valores adyacentes a lo largo de s
	//y buscamos donde sea menor de 0
	signed int *diffS;
	diffS = malloc(length(s)*sizeof(int));
	//int *iMax;
	//iMax = malloc(length(s)*sizeof(int));;

	struct fifo_queue iMax;
	init_fifo_queue( &iMax, length(diffS));

	for(int i=0; i<=length(diffS); i++){
		if(i>0){
			diffS[i-1] = iTemp(i-1)-iTemp(i);
		}
	}

	for(int i=0; i<=length(diffS); i++){
		if(diffS[i]<0){
			enqueue(&iMax, i+1);
		}
	}

	//Los indexamos en el array de valores
	struct fifo_queue iPk;
	init_fifo_queue( &iPk, iMax->size);
	for(int i=0; i<=iMax->size; i++){
		enqueue(&iPk, i-1);
	}

	//findPeaksSeparatedByMoreThanMinPeakDistance
	//guardamos peaks y locs en dos variables
	float pks[iMax->size];
	int locs[iMax->size];
	locs=iPk;

	for(int i=0; i<=iMax->size; i++){
			pks[i]=yTemp[iMax[i]];

		}

	//ordenamos los Peaks de mayor a menor y guardamos
	//los idx ordenados tmbn en otro array
	struct sort
	{
		float value;
		int idx;
	};

	struct sort sortIdx[iMax->size];
	for (int i = 0; i < iMax->size; i++)
	    {
	        sortIdx[i].value = pks[i];
	        sortIdx[i].idx = i;
	    }
	int comp(const void *a, const void *b)
	{
	    struct str *a1 = (struct str *)a;
	    struct str *a2 = (struct str *)b;
	    if ((*a1).value > (*a2).value)
	        return -1;
	    else if ((*a1).value < (*a2).value)
	        return 1;
	    else
	        return 0;
	}
	 qsort(sortIdx, iMax->size, sizeof(sortIdx[0]), comp);//decending function
	 //para comprobar
	 //for (int i = 0; i < iMax->size; i++)
	//	 printf("%d ", sortIdx[i].idx); //will give 1 2 0

	 //una vez tenemos el array de idx ordenado obtenemos el
	 //array de x indexando sortIdx

	 for(int i=0; i<=length(sortIdx); i++){
		 locs[i] = sortIdx[i].idx;
	 }

	 int idelete[length(locs)];
	 for(int i=0; i<=length(locs); i++){
	 		idelete[i]=0;
	 	}

	//If the peak is not in the neighborhood of a larger peak, find
    //secondary peaks to eliminate.
	 int masc1[length(locs)];
	 int masc2[length(locs)];

	 for(int i=0; i<=length(locs); i++){
	   if(!idelete(i)){
		 for(int k=0; k<=length(locs); k++){
			if(locs[k]>=(locs[i]-env->minD)){
				masc1[k]=1;
			} else masc1[k]=0;

			if(locs[k]<=(locs[i]+env->minD)){
				masc2[k]=1;
			} else masc2[k]=0;

		 }
		 idelete = idelete | (masc1 & masc2);
		 idelete[i] = 0; //keep current peak

	   }
	 }

	 //Una vez tenemos seleccionados los maximos los volvemos a reordenar
	 //en orden cronologico
	 int comp2(const void *a, const void *b)
	 {
	     struct str *a1 = (struct str *)a;
	     struct str *a2 = (struct str *)b;
	     if ((*a1).idx > (*a2).idx)
	         return 1;
	     else if ((*a1).idx < (*a2).idx)
	         return -1;
	     else
	         return 0;
	 }
	 qsort(sortIdx, iMax->size, sizeof(sortIdx[0]), comp2);

	 //Una vez lo tenemos reordenado guardamos los idx

	 //int idx[iMax->size];
	 for(int i=0; i<=iMax->size; i++){
	 	 		//idx[i]=sortIdx[i].value;
		 env->y[i]=sortIdx[i].value;
		 env->x[i]=sortIdx[i].idx;
	 	 	}



}
