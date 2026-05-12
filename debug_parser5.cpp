#include <iostream>
#include "sql_lexer.h"
#include "sql_parser.h"

using namespace SQL;

// Add friend class to access private methods for debugging
class DebugParser : public Parser {
public:
    DebugParser(const std::vector<Token>& tokens) : Parser(tokens) {}
    
    void debugParseCreateTable() {
        std::cout << "Starting parseCreateTable simulation:\n";
        
        // We can't directly call private methods, but we can trace through the logic
        // After parsing CREATE TABLE users, current token should be '(' at position 3
        
        // But wait - the error says "Expected '(' after table name" and token is 'id'
        // This means after reading table name, currentToken is 'id', not '('
        
        // Let's check if parseColumnList is being called instead of parseCreateTable!
    }
};

int main() {
    std::string sql = "CREATE TABLE users (id LONG)";
    std::cout << "SQL: " << sql << "\n\n";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    
    std::cout << "Tokens:\n";
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        std::cout << "[" << i << "] type=" << static_cast<int>(token.type) 
                  << " (" << tokenTypeToString(token.type) << ") | '" << token.value << "'\n";
    }
    
    std::cout << "\nFirst token: " << tokenTypeToString(tokens[0].type) << "\n";
    std::cout << "TokenType::CREATE value: " << static_cast<int>(TokenType::CREATE) << "\n";
    
    return 0;
}
