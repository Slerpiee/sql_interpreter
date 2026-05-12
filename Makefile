CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
LDFLAGS =

all: sql_demo sql_executor_test

sql_demo: test_sql.cpp sql_lexer.cpp sql_parser.cpp sql_tokens.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

sql_executor_test: test_executor.cpp sql_lexer.cpp sql_parser.cpp sql_tokens.cpp sql_semantic.cpp sql_executor.cpp Table.c
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f sql_demo sql_executor_test *.o *.dat

run: sql_demo
	./sql_demo

run_executor: sql_executor_test
	./sql_executor_test

.PHONY: all clean run run_executor
