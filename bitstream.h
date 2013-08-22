#include <stdio.h>

typedef struct BITFILE BITFILE;

int bfclose(BITFILE *bf);
int bfflush(BITFILE *bf);
BITFILE *bfopen(FILE *f);
uint32_t bfread(int bits, BITFILE *bf, int *err);
int bfseek_r(BITFILE *bf, long offset, int bits, int whence);
int bfwrite(uint32_t data, int bits, BITFILE *bf);
