#include <stdio.h>
#include <string.h>


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

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Uso: %s string\n", argv[0]);
		return(1);
	}

	char crc = crc8(argv[1], strlen(argv[1]));

	printf("%02Xh '%c'\n", crc, crc);
	
	return(0);
}
