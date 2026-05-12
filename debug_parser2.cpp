#include <iostream>
#include "sql_lexer.h"
#include "sql_parser.h"

using namespace SQL;

int main() {
    std::string sql = "CREATE TABLE users (id LONG, name TEXT(50))";
    std::cout << "SQL: " << sql << "\n\n";
    
    try {
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        std::cout << "Tokens:\n";
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            std::cout << "[" << i << "] " << tokenTypeToString(token.type) << " | '" << token.value << "'\n";
        }
        
        std::cout << "\nParsing step by step:\n";
        Parser parser(tokens);
        
        // currentToken() should be CREATE at start
        std::cout << "Current token before parse: " << tokenTypeToString(parser.currentToken().type) << "\n";
        
    } catch (const ParserException& e) {
        std::cout << "Parser error: " << e.what() << " at line " << e.getToken().line << ", col " << e.getToken().column << "\n";
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    
    return 0;
}
