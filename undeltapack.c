#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include "bitstream.h"

BITFILE *bf;

int32_t unpack(int packbits, int *err) {
	int32_t ret = 0;
	int32_t neg = 0;
	int bits = 0;
	uint32_t val;

	int readerr = 0;

	val = bfread(packbits + 1, bf, &readerr);
	if(readerr) {
		*err = EOF;
		return 0;
	}
	if(val & (1<<(packbits-1))) {
		neg = 1;
	}
	ret |= val & ((1<<(packbits-1))-1);
	bits = packbits - 1;
	while((val & (1<<packbits)) == 0) {
		val = bfread(packbits + 1, bf, &readerr);
		if(readerr) {
			*err = EOF;
			return 0;
		}
		ret  |= (val & ((1<<packbits)-1)) << bits;
		bits += packbits;
	}
	if(neg) {
		ret = -ret;
	}
	return ret;
}

int handleblock(int packbits) {
	int bufsize;
	int32_t *diffs;
	int i;

	struct tm stm;
	char ctm[32];

	int err = 0;

	bufsize = bfread(32, bf, &err);
	if(err != 0) {
		return EOF;
	}

	diffs = malloc(bufsize * 3 * sizeof(int32_t));

	for(int j = 0; j < 3; j++) {
		for(i = 0; i < bufsize; i++) {
			diffs[i*3+j] = unpack(packbits, &err);
			if(err != 0) {
				goto error;
			}
		}
	}

	for(int i = 3; i < 3*bufsize; i+=3) {
		diffs[i+0] += diffs[i-3];
		diffs[i+1] += diffs[i-2];
		diffs[i+2] += diffs[i-1];
	}
	for(int i = 0; i < bufsize; i++) {
		time_t tm = (time_t) diffs[3*i];
		localtime_r(&tm, &stm);
		strftime(ctm, sizeof ctm, "%Y-%m-%d_%H:%M:%S", &stm);

		printf("%s %02d.%1d %02d.%1d\n",
			   ctm,
			   diffs[3*i+1] / 10, diffs[3*i+1] % 10,
			   diffs[3*i+2] / 10, diffs[3*i+2] % 10);
	}
	return 0;

 error:
	free(diffs);
	return EOF;
}

int main() {
	int err = 0;
	bf = bfopen(stdin);
	int packbits = bfread(8, bf, &err);
	if(err != 0) {
		return 1;
	}
	while(handleblock(packbits) != EOF);
	bfclose(bf);
	return 0;
}
