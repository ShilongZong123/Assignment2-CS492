CFLAGS=-O2 -Wall -I../driver
TGT=scull


.PHONY: clean

all: $(TGT)

$(TGT): scull.c
	gcc scull.c -lpthread -I../driver -o $(TGT)

clean:
	rm -f *.o $(TGT)
