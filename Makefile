CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
LDFLAGS =

all: sql_demo

sql_demo: test_sql.cpp sql_lexer.cpp sql_parser.cpp sql_tokens.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f sql_demo *.o

run: sql_demo
	./sql_demo

.PHONY: all clean run
