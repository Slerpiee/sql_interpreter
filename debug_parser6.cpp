#include <iostream>
#include "sql_lexer.h"
#include "sql_parser.h"

using namespace SQL;

int main() {
    // Test simple SELECT first to verify parser works
    std::string sql1 = "SELECT * FROM users";
    std::cout << "SQL: " << sql1 << "\n\n";
    
    try {
        Lexer lexer(sql1);
        auto tokens = lexer.tokenize();
        
        Parser parser(tokens);
        auto cmd = parser.parse();
        
        if (std::holds_alternative<std::unique_ptr<SelectStmt>>(cmd)) {
            std::cout << "SELECT parsed successfully!\n\n";
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n\n";
    }
    
    // Now test CREATE TABLE
    std::string sql2 = "CREATE TABLE users (id LONG)";
    std::cout << "SQL: " << sql2 << "\n\n";
    
    try {
        Lexer lexer(sql2);
        auto tokens = lexer.tokenize();
        
        std::cout << "Tokens:\n";
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            std::cout << "[" << i << "] type=" << static_cast<int>(token.type) 
                      << " (" << tokenTypeToString(token.type) << ") | '" << token.value << "'\n";
        }
        
        Parser parser(tokens);
        auto cmd = parser.parse();
        
        std::cout << "CREATE TABLE parsed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    
    return 0;
}
