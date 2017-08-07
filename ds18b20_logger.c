/*
 * ds18b20.c
 *      Prueba de ds18b20 usando la librería wiringpi
 *      Reinoso G. 07/08/2017
 *
 *      gcc -lwiringPi -o ds18b20_logger ds18b20_logger.c
 */
 
#include <stdio.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <time.h>

#define PIN 1         /* Pin number */
#define TIMESLOT 90   /* 1-Wire time slot in us 60-120 */
#define TREC 15       /* Line pull up recovery time us */


/* Pulls down the bus for given us and then releases it */
void low(int pin, int us) {
	pinMode (pin, OUTPUT);
    digitalWrite (pin, LOW);
	delayMicroseconds (us);
	pinMode (pin, INPUT);
}

/* Sends a 0 by pulling the bus for a whole time slot
 * Sends a 1 by pulling just a bit (10-15ms),
 * and then releasing it for the rest of time slot */
void send_bit(int pin, int bit) {
	if (bit) {
		low(pin,5); /* 1-5 us */
		delayMicroseconds(TIMESLOT-5);
	}
	else {
		low(pin,TIMESLOT);
	}
}

/* Sends 8 bit in a row, LSB first */
void send_byte(int pin, char byte) {
	int i;

	for (i = 0; i < 8; i++) {
		send_bit(pin, byte & 1);
		byte = byte >> 1;
		delayMicroseconds(TREC);
	}
}

/* Sends a brief pulse and then read for the response */
int read_bit(pin) {
	int s;

	low(pin, 5);
	delayMicroseconds(TREC);
	s = digitalRead(pin);
	delayMicroseconds(TIMESLOT-5);

	//printf("%s", s ? "1" : "0");
	return s;
}

/* Reads a byte, LSB first */
char read_byte(pin) {
	int byte = 0x00;
	int i;

	for (i=0; i<8; i++) {
	    int b;
		b = read_bit(pin);
		b = b << i;
		byte = byte | b;
		delayMicroseconds(TREC);
	}

	return byte;
}

/* Wait until line goes HIGH */
void wait_line(int pin) {
	while (!digitalRead(pin))
		delayMicroseconds(TIMESLOT);
}


/* Sends a reset pulse and waits for a presence response */
int reset (int pin) {
	int v;
	low (pin, 600);                      /* Reset pulse 480-960us */
	delayMicroseconds (45);              /* Wait 15-60 and answer back*/
	v = digitalRead(pin);                /* DS1820 pulls down if present */
	wait_line(pin);
	return !v;
}

/* Calculates CRC8-Maxim according datasheet
 * If CRC is appended at the end of string, correct array gives result 00 */
unsigned char crc8 (char *str, size_t len) {
	char div = 0b10001100; // Rotated poly
	unsigned char crc = 0;

	size_t i;
	for (i = 0; i < len; i++) {
		unsigned char byte = str[i];

		int j;
		for (j = 0; j < 8; j++) {

			// Shift CRC
			char crc_carry = crc & 1;
			crc >>= 1;

			// Shift Byte
			char byte_carry = byte & 1;
			byte >>= 1;

			// If crc_carry XOR byte_carry we make crc XOR div
			if (crc_carry ^ byte_carry)
				crc ^= div;
		}

	}

	return crc;
}

/* Reads ROM, void function just for testing */
void read_rom(int pin) {
	puts ("Reading ROM data (Cmd 33h)");
	send_byte(pin, 0x33);
	char rom_data[8];

	int i;
	for (i = 0; i < 8; i++) {
		rom_data[i] = read_byte(pin);
		printf("%02x ", rom_data[i]);
	}

	if (crc8(rom_data, 8)) {
		puts("\tROM data read ERROR, try again.");
		exit(0);
	}
	else {
		puts("\tOK");
	}

	
	reset(pin);
}

/* Starts a temperature convertion and waits for it to be done */
void convert_t (int pin) {
	send_byte(pin, 0xCC);
	send_byte(pin, 0x44);

	while (read_byte(pin) != 0xFF)
		delayMicroseconds(50);

	reset(PIN);
}

/* Reads the whole scratchpad to the buffer (9 bytes at least) */
int read_scratchpad(int pin, char *buff) {
    do {
		send_byte(PIN, 0xCC);
		send_byte(PIN, 0xBE);

		int i;
		for (i = 0; i < 9; i++) {
			buff[i] = read_byte(PIN);
			//printf("%02x ", buff[i]);
		}
		/*
		if (crc8(buff, 9)) {
			puts("\tScratchpad data read ERROR, try again.");
		}
		else {
			puts("\tOK");
		}
		*/
		reset(PIN);

	} while(crc8(buff, 9));

	return 0;
}

struct timeval GetTimeStamp() {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return tv;
}



int main (void)
{


	if (wiringPiSetup () == -1)
		return 1;
 
	piHiPri (99); /* us timing requires near real-time */
	pinMode (PIN, INPUT);
	pullUpDnControl (PIN, PUD_UP);


	if (!reset(PIN)) 
		return 1;

	convert_t(PIN);

	char scratchpad[9];
	read_scratchpad(PIN, scratchpad);

	int16_t temp = (scratchpad[1] << 8) + scratchpad[0];

	struct timeval tv = GetTimeStamp();

	printf ("%u.%03u\t%.2f\n",
		tv.tv_sec, tv.tv_usec/1000,
		(float) temp / 16);

	return 0;
}
