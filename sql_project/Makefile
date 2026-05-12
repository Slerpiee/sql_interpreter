CXX      = g++
CC       = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -g
CFLAGS   = -std=c99 -Wall -g -DS_IREAD=S_IRUSR -DS_IWRITE=S_IWUSR

all: client server

# C table driver (compiled as C)
Table.o: Table.c _Table.h
	$(CC) $(CFLAGS) -c Table.c -o Table.o

# Server binary
server: server.cpp Table.o _Table.h Table.hpp SQLParser.hpp SQLServer.hpp Protocol.hpp ResultFormatter.hpp
	$(CXX) $(CXXFLAGS) server.cpp Table.o -o server

# Client binary
client: client.cpp Table.o _Table.h Table.hpp Protocol.hpp ResultFormatter.hpp
	$(CXX) $(CXXFLAGS) client.cpp Table.o -o client

clean:
	rm -f *.o client server *.dat

run: all
	./client

.PHONY: all clean run
