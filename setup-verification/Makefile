CC=clang++
CFLAGS=-g -Wall -I.
LIBS=-lglfw -lvulkan

main: main.o
	$(CC) $(LIBS) main.o -o main

main.o: main.cc
	$(CC) $(CFLAGS) -c main.cc -o main.o

clean :
	rm main main.o
