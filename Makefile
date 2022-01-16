CC=g++
SRC=src
CFLAGS=-std=c++14 -Wall -Iinclude -Isrc
LDFLAGS=-lraylib -lopengl32 -lgdi32 -lwinmm

pcg:
	$(CC) -shared -o pcg.dll $(SRC)/pcg.cpp $(CFLAGS)

main:
	$(CC) -o main.exe $(SRC)/main.cpp $(CFLAGS) $(LDFLAGS)
