CC = g++

CFLAGS = -Wall -std=c++11 -ggdb -O3

all: main.cpp
	$(CC) $(CFLAGS) main.cpp $(OBJS) -o subdivide $(LIBS)

