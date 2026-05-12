CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS =

# Исходники
SERVER_SRC = server.cpp interpreter.cpp semantic_analyzer.cpp sql_parser.cpp sql_lexer.cpp sql_tokens.cpp table_lib.c
CLIENT_SRC = client.cpp sql_parser.cpp sql_lexer.cpp sql_tokens.cpp sql_tokens.cpp

all: server client

server: $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

client: $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f server client *.dat /tmp/sql_server.sock

run-server: server
	./server

run-client: client
	./client

.PHONY: all clean run-server run-client
