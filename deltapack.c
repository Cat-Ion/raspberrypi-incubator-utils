#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "bitstream.h"

#define BUF 1024
#define PACKBITS 3

BITFILE *bf;

/* Packs an int into a series of one or more PACKBITS+1-sized
   packets. The MSB being set indicates that this is the last
   packet. After that follows a sign bit and the PACKBITS-1 LSB (in
   the first packet) of abs(x), or the PACKBITS next-higher bits of
   abs(x) in the following packets.
 */
int pack(int32_t x) {
	uint32_t b = 0;
	/* Compute sign bit */
	if(x < 0) {
		b |= 1 << (PACKBITS-1);
		x = -x;
	}
	/* First PACKBITS-1 bits */
	b |= x & ((1<<(PACKBITS-1))-1);
	x >>= PACKBITS - 1;
	/* While we haven't handled all of x */
	while(x) {
		/* Write the current bit */
		if(bfwrite(b, PACKBITS + 1, bf) == EOF) {
			return EOF;
		}
		/* Create a new one with the PACKBITS smallest bits of the
		   remaining x, then shift them out of x */
		b = x & ((1<<PACKBITS)-1);
		x >>= PACKBITS;
	}
	/* Set the finished indicator bit */
	b |= 1<<PACKBITS;
	if(bfwrite(b, PACKBITS + 1, bf) == EOF) {
		return EOF;
	}
	return 0;
}

/* Encodes a block of up to BUF samples.

   The first output is a 32-bit integer showing the size of the
   block. After that follow 3 blocks of that size containing the
   delta-encoding of the timestamp, then the humidity data, and
   the temperature data. Returns the block size or EOF on error.
 */
int handleblock() {
	char buf[64];
	int ret;
	int32_t i;
	
	struct tm tm = (struct tm) {0};
	tm.tm_isdst = -1;

	int32_t diffs[BUF][3];

	time_t oldtime, curtime;
	int32_t hum, humd, temp, tempd;
	int32_t oldhum, curhum, oldtemp, curtemp;

	/* Read up to BUF samples into the diffs[] array */
	if(fgets(buf, 64, stdin) == NULL) {
		return EOF;
	}
	ret = sscanf(buf, "%d-%d-%d_%d:%d:%d %d.%d %d.%d\n",
				&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
				&tm.tm_hour, &tm.tm_min, &tm.tm_sec,
				&hum, &humd,
				&temp, &tempd);
	if(ret != 10) {
		return EOF;
	}

	tm.tm_year -= 1900;
	tm.tm_mon -= 1;

	oldtime = mktime(&tm);
	oldhum = hum * 10 + humd;
	oldtemp = temp * 10 + tempd;

	diffs[0][0] = oldtime;
	diffs[0][1] = oldhum;
	diffs[0][2] = oldtemp;

	for(i = 1; i < BUF; i++) {
		if(fgets(buf, 64, stdin) == NULL) {
			/* End of file, stop */
			break;
		}
		ret = sscanf(buf, "%d-%d-%d_%d:%d:%d %d.%d %d.%d\n",
					&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
					&tm.tm_hour, &tm.tm_min, &tm.tm_sec,
					&hum, &humd,
					&temp, &tempd);
		if(ret != 10) {
			if(ret == 0) {
				/* Maybe an empty line */
				break;
			} else {
				/* Other crap */
				return EOF;
			}
		}

		tm.tm_year -= 1900;
		tm.tm_mon -= 1;

		curtime = mktime(&tm);
		curhum = hum * 10 + humd;
		curtemp = temp * 10 + tempd;

		diffs[i][0] = curtime - oldtime;
		diffs[i][1] = curhum - oldhum;
		diffs[i][2] = curtemp - oldtemp;

		oldtime = curtime;
		oldhum  = curhum;
		oldtemp = curtemp;
	}
	/* Write the block size */
	bfwrite(i, 32, bf);
	/* Pack the timestamps, then the humidity, then the
	   temperature. */
	for(int j = 0; j < 3; j++) {
		for(int k = 0; k < i; k++) {
			if(pack(diffs[k][j]) == EOF) {
				return EOF;
			}
		}
	}
	return i;
}

int main() {
	bf = bfopen(stdout);
	bfwrite(PACKBITS, 8, bf);
	while(handleblock() != EOF);
	bfclose(bf);
	return 0;
}
