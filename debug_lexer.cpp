#include <iostream>
#include "sql_lexer.h"

using namespace SQL;

int main() {
    std::string sql = "CREATE TABLE users (id LONG, name TEXT(50), age LONG, email TEXT(100))";
    std::cout << "SQL: " << sql << "\n\n";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    
    for (const auto& token : tokens) {
        std::cout << "Token: " << static_cast<int>(token.type) << " | Value: '" << token.value << "'\n";
    }
    
    return 0;
}
