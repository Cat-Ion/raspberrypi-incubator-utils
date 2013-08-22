CFLAGS+=-std=c99 -pedantic -Wall -Os -D_POSIX_C_SOURCE=1

BIN=deltapack undeltapack

all: $(BIN)

deltapack: deltapack.o bitstream.o

undeltapack: undeltapack.o bitstream.o

clean:
	rm -f $(BIN) *.o

.PHONY: clean
