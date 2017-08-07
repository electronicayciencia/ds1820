/*
 * ds18b20.c
 *      Prueba de ds18b20 usando la librería wiringpi
 *      Reinoso G. 07/08/2017
 *
 *      gcc -lwiringPi -o ds18b20 ds18b20.c
 */
 
#include <stdio.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <stdlib.h>

#define PIN 1         /* Pin number */
#define TIMESLOT 90   /* 1-Wire time slot in us 60-120 */
#define TREC 10       /* Line pull up recovery time us */



/* Pulls down the bus for given us and then releases it */
void low(int pin, int us) {
	pinMode (pin, OUTPUT);
    digitalWrite (pin, LOW);
	delayMicrosecondsHard (us);
	pinMode (pin, INPUT);
	delayMicrosecondsHard(TREC);
}

/* Sends a 0 by pulling the bus for a whole time slot
 * Sends a 1 by pulling just a bit (10-15ms),
 * and then releasing it for the rest of time slot */
void send_bit(int pin, int bit) {
	if (bit) {
		low(pin,2); /* 1-5 us */
		delayMicrosecondsHard(TIMESLOT-5);
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
	}
}

/* Sends a brief pulse and then read for the response */
int read_bit(pin) {
	int s;

	low(pin, 2);  // > 1us
	//delayMicrosecondsHard(TREC);
	s = digitalRead(pin);
	delayMicrosecondsHard(TIMESLOT);

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
		//delayMicrosecondsHard(TREC);
	}

	return byte;
}


/* Sends a reset pulse and waits for a presence response */
int reset (int pin) {
	int v;
	low (pin, 960);                      /* Reset pulse 480-960us */
	delayMicrosecondsHard(60);          /* Wait 15-60 and answer back*/
	v = digitalRead(pin);                /* DS1820 pulls down if present */
	delayMicroseconds(300);          /* Wait  and answer back*/
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
		delayMicroseconds(20000);  // up to 500ms

	reset(pin);
}

/* Reads the whole scratchpad to the buffer (9 bytes at least) */
void read_scratchpad(int pin, char *buff) {
//    do {
		send_byte(PIN, 0xCC);
		send_byte(PIN, 0xBE);

		int i;
		for (i = 0; i < 9; i++) {
			buff[i] = read_byte(PIN);
			printf("%02x ", buff[i]);
		}
		
		if (crc8(buff, 9)) {
			puts("\tScratchpad data read ERROR, try again.");
		}
		else {
			puts("\tOK");
		}
		
		reset(PIN);

//	} while(crc8(buff, 9));

}


int main (void)
{

	puts ("DS1820 Test Program for Raspberry Pi");

	if (wiringPiSetup () == -1)
		return 1;
 
	piHiPri (99); /* us timing requires near real-time */
	pinMode (PIN, INPUT);
	pullUpDnControl (PIN, PUD_UP);

	if (reset(PIN)) {
		puts ("Device present!");
	}
	else {
		puts ("Device not present, no response :(");
		return 1;
	}

	read_rom(PIN);

	puts("Reading temperature:");
	for (;;) {
		convert_t(PIN);

		char scratchpad[9];
		read_scratchpad(PIN, scratchpad);

		short int temp_read    = scratchpad[0];
		short int count_remain = scratchpad[6];
		short int count_per_c  = scratchpad[7];

		/* Low res temp */
		//float temp = (float) temp_read / 2;
		printf("Temperature (low res) is %.1fºC\n", (float)temp_read/2);

		/* High res temp */
		float temp_hr = (int) temp_read / 2;
		temp_hr = temp_hr - 0.25 + ((float)count_per_c - (float)count_remain) / (float)count_per_c;

		printf("COUNT_PER_C: %d, COUNT_REMAIN: %d\n", count_per_c, count_remain);
		printf("Temperature (hi res) is %.2fºC\n", temp_hr);

		

	}

	return 0;
}
