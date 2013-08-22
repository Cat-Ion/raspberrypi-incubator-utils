#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "bitstream.h"

struct BITFILE {
	FILE *f;
	
	/* Output */
	uint32_t w_active_byte; /* Unwritten output in the lower w_bits bits */
	int w_bits;             /* Number of bits of unwritten output */

	/* Input */
	uint8_t r_active_byte;  /* Input that hasn't been returned in the lower r_bits bits */
	uint8_t r_bits;         /* Number of bits in the input buffer */
};

struct BITFILE *bfopen(FILE *f) {
	struct BITFILE *bf = malloc(sizeof (struct BITFILE));
	if(bf == NULL) {
		return NULL;
	}
	bf->f = f;
	bf->w_active_byte = bf->w_bits = 0;
	bf->r_active_byte = bf->r_bits = 0;
	return bf;
}

int bfflush(struct BITFILE *bf) {
	bf->w_active_byte <<= 8-bf->w_bits;
	if(fputc(bf->w_active_byte & 0xFF, bf->f) == EOF) {
		return EOF;
	}
	if(fflush(bf->f) == EOF) {
		return EOF;
	}
	return 0;
}

int bfclose(struct BITFILE *bf) {
	int ret;
	bfflush(bf);
	ret = fclose(bf->f);
	if(ret == EOF) {
		return ret;
	}
	free(bf);
	return ret;
}

int bfwrite(uint32_t data, int bits, struct BITFILE *bf) {
	int written = 0;
	if(bf->w_bits != 0 && (bits + bf->w_bits) >= 8) {
		bf->w_active_byte <<= 8-bf->w_bits;
		bf->w_active_byte  |= (data >> (bits - 8 + bf->w_bits)) & ((1<<(8 - bf->w_bits))-1);
		if(fputc(bf->w_active_byte & 0xFF, bf->f) == EOF) {
			return EOF;
		}
		written++;
		bits -= 8 - bf->w_bits;
		bf->w_active_byte = 0;
		bf->w_bits = 0;
	}
	while(bits >= 8) {
		bf->w_active_byte = (data >> (bits - 8)) & 0xFF;
		if(fputc(bf->w_active_byte & 0xFF, bf->f) == EOF) {
			return EOF;
		}
		written++;
		bits -= 8;
		bf->w_active_byte = 0;
	}
	if(bits > 0) {
		bf->w_active_byte <<= 8;
		bf->w_active_byte  |= data & ((1U<<bits)-1);
		bf->w_bits += bits;
	}
	return written;
}

uint32_t bfread(int bits, struct BITFILE *bf, int *err) {
	uint32_t ret = 0;
	int c = 0;
	if(bf->r_bits >= bits) {
		ret = (bf->r_active_byte >> (bf->r_bits - bits)) & ((1<<bits)-1);
		bf->r_bits -= bits;
		bf->r_active_byte &= ((1<<bf->r_bits)-1);
		return ret;
	}
	ret = bf->r_active_byte;
	bits -= bf->r_bits;
	bf->r_bits = 0;
	while(bits >= 8) {
		c = fgetc(bf->f);
		if(c == EOF) {
			*err = EOF;
			return 0;
		}
		ret <<= 8;
		ret  |= c;
		bits -= 8;
	}
	if(bits > 0) {
		c = fgetc(bf->f);
		if(c == EOF) {
			*err = EOF;
			return 0;
		}
		bf->r_active_byte = c & ((1<<(8-bits))-1);
		bf->r_bits = 8-bits;
		c   >>= 8-bits;
		ret <<= bits;
		ret  |= c;
	}
	return ret;
}

int bfseek_r(struct BITFILE *bf, long offset, int bits, int whence) {
	int err = 0;
	if(bits < 0) {
		offset += bits / 8 - 1;
		bits = (bits % 8) + 8;
	}
	switch(whence) {
	case SEEK_SET:
		if(fseek(bf->f, offset + bits / 8, SEEK_SET) == -1) {
			return -1;
		}
		
		bfread(bits % 8, bf, &err);
		if(err == EOF) {
			return -1;
		}
		return 0;
	case SEEK_CUR:
		if(fseek(bf->f, offset + (bf->r_bits + bits) / 8, SEEK_SET) == -1) {
			return -1;
		}
		bfread((bf->r_bits + bits) % 8, bf, &err);
		if(err == EOF) {
			return -1;
		}
		return 0;
	case SEEK_END:
		if(fseek(bf->f, offset + (bits + 7) / 8, SEEK_SET) == -1) {
			return -1;
		}
		bfread((8-bits) % 8, bf, &err);
		if(err == EOF) {
			return -1;
		}
		return 0;
	default:
		return -1;
	}
	
}
