/* Leer el fichero monitor.bin y escribe el resultado en monitor.dat */

#include <time.h>
#include <sys/time.h>
#include <stdio.h>


struct monitor {
	unsigned int tv_sec;
	unsigned int tv_usec;
	unsigned int status;
	unsigned int pulled;
};


int main(void) {
	char buffer[sizeof(struct timeval) + sizeof(int)];

	FILE *fbin = fopen("monitor.bin", "rb");
	FILE *fdat = fopen("monitor.dat", "w");

	if (! fbin) {
		puts("El fichero monitor.bin no existe o no se puede leer.");
		return 0;
	}
	if (! fdat) {
		puts("El fichero monitor.dat no se puede escribir.");
		return 0;
	}

	int max_step = 0;
	int max_step_ln = 0;
	int last_us = -1;
	int n = 1;
	while(1) {
		struct monitor m;
		int step;

		if (!fread((void *) &m, sizeof(m), 1, fbin)) break;

		if (m.pulled > 1 || m.status > 1) {
			puts("Archivo 'monitor.bin' corrupto o no válido.");
			return 0;
		}

		if (last_us < 0) last_us = m.tv_usec;

		step = m.tv_usec - last_us;

		if (step > max_step) {
			max_step = step;
			max_step_ln = n;
		}
		if (step < 0) {
			printf("Diferencia negativa (%d) linea %d.\n", step, n);
		}

		fprintf(fdat, "%d\t%d\t%d\t%d\n", m.tv_sec, m.tv_usec, m.status, m.pulled);

		n++;
		last_us = m.tv_usec;
	}

	fclose(fbin);
	fclose(fdat);

	printf("Max step: %d us, linea %d\n", max_step, max_step_ln);
}




