#include <iostream>
#include "sql_lexer.h"
#include "sql_parser.h"

using namespace SQL;

int main() {
    std::string sql = "CREATE TABLE users (id LONG)";
    std::cout << "SQL: " << sql << "\n\n";
    
    try {
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        std::cout << "Tokens:\n";
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            std::cout << "[" << i << "] type=" << static_cast<int>(token.type) << " | '" << token.value << "'\n";
        }
        
        std::cout << "\nParsing...\n";
        Parser parser(tokens);
        auto cmd = parser.parse();
        
        std::cout << "Success!\n";
        
    } catch (const ParserException& e) {
        std::cout << "Parser error: " << e.what() << "\n";
        std::cout << "Token: " << e.getToken().value << " at line " << e.getToken().line << ", col " << e.getToken().column << "\n";
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    
    return 0;
}
