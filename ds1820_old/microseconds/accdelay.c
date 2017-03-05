/*
 * Prueba para comprobar cuando microsegundos espera realmente.
 */
 
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <wiringPi.h>
#include <math.h>

#define MIN 0
#define MAX 505
#define TIMES 1000
#define INCR 50

// Para histograma
#define DIFFMAX 60
#define BINS 1

// Tiempos: 5us 10us 15us ... 750us
#define TBUCKETS (MAX-MIN)/INCR+1

int   t0[TBUCKETS]         = {0};
int   n[TBUCKETS]          = {0};
int   min[TBUCKETS]        = {0};
int   max[TBUCKETS]        = {0};
float avg[TBUCKETS]        = {0};
float var[TBUCKETS]        = {0};
float delta[TBUCKETS]      = {0};
int   hist[TBUCKETS][BINS] = {0};


int accdelay(unsigned long us) {
	unsigned long start, now;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	now   = 1000000 * tv.tv_sec + tv.tv_usec;
	start = now;

	int n = 0;
	while(1) {
		gettimeofday(&tv,NULL);
		now   = 1000000 * tv.tv_sec + tv.tv_usec;

		if (now - start >= us) {
		  break;
		}

		if (us - (now - start) > 500) {
			struct timespec tim;
			tim.tv_sec = 0;
			tim.tv_nsec = 50000;

			nanosleep(&tim , NULL);
		}
	}

	return 0;
}


int microsleep(long us)
{
	struct timespec tim, tim2;
	tim.tv_sec = 0;
	tim.tv_nsec = us*1000;

	return nanosleep(&tim , NULL);
}



int time_diff(struct timeval x , struct timeval y)
{
	double x_ms , y_ms , diff;
		
	x_ms = x.tv_sec*1000000 + x.tv_usec;
	y_ms = y.tv_sec*1000000 + y.tv_usec;
				
	diff = y_ms - x_ms;
					
	return diff;
}

int main (void)
{

	int i;
	for (i=0;i<=TBUCKETS-1;i++)
		min[i] = 999999;


//	if (wiringPiSetup () == -1)
//		return 1;
 
//	piHiPri (99); /* us timing requires near real-time */

	int j;
	for (j=0; j < TIMES; j++) {

		if (j % 100 == 0)
			fprintf(stderr, "Iteración %d de %d...\n", j, TIMES);

		int i;
		for (i=0;i<TBUCKETS-1;i++) {
		
			int t;

			int t_req = MIN + INCR * i;
			t0[i] = t_req;

			struct timeval before , after;
			gettimeofday(&before , NULL);

			//delayMicroseconds(t_req);
			delayMicrosecondsHard(t_req);
			//accdelay(t_req);
			//microsleep(t_req);

			gettimeofday(&after , NULL);

			t = time_diff(before, after);

			t = t - t_req; // la diferencia es lo que nos interesa

			n[i]++;
			
			delta[i] = t - avg[i];
			avg[i]  += delta[i] / n[i];
			var[i]  += delta[i]*(t - avg[i]);

			if (t > max[i]) max[i] = t;
			if (t < min[i]) min[i] = t;

			//printf("%d_%d -> %d\n",t_req, j, t);
			// Partimos de que siempre el tiempo es superior
			// o igual al solicitado
			int bin; 
			bin = t * (BINS-1) / DIFFMAX;

			if (bin > BINS-1) bin = BINS-1;
			hist[i][bin]++;

		}

	}

	printf("%s\t%s\t%s\t%s\t%s", "t", "min", "avg", "max", "var");
	for (i=0;i<BINS;i++)
		printf("\tbin%d", i+1);

	printf("\n");
	
	
	for (i=0;i<TBUCKETS-1;i++) {
		printf("%d\t%d\t%.2f\t%d\t%0.2f", 
			t0[i], 
			min[i], 
			avg[i],
			max[i],
			var[i] / (n[i]-1)
		);

		int j;	
		for (j=0;j<BINS;j++)
			printf("\t%d", hist[i][j]);

		printf("\n");
	}

	return 0;
}
