CC = gcc
CFLAGS = -Wall -g

mydiz: mydiz.o flags.o implementations.o structs.o
	$(CC) $(CFLAGS) -o mydiz mydiz.o flags.o implementations.o structs.o

mydiz.o: mydiz.c
	$(CC) $(CFLAGS) -c mydiz.c

flags.o: flags.c
	$(CC) $(CFLAGS) -c flags.c

implementations.o: implementations.c
	$(CC) $(CFLAGS) -c implementations.c

structs.o: structs.c
	$(CC) $(CFLAGS) -c structs.c

.PHONY: clean

clean:
	rm -f mydiz.o flags.o implementations.o structs.o