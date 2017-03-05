#include <stdio.h>

void main (void) {
	int v[] = {0,1,2,3,4,5,6,7,8,9};

	int n = 0;
	float mean = 0;
	float m2 = 0;

	int i;
	for(i=0; i<10; i++) {
	  	int t = v[i];

		float delta = 0;
		n++;
		delta = t - mean;
		mean += delta / n;
		m2 += delta*(t - mean);
	}

	m2 /= (n-1);

	printf ("n: %d, mean: %.2f, m2: %.2f\n", n, mean, m2);
}

