#include <iostream>
#include "sql_lexer.h"
#include "sql_parser.h"

using namespace SQL;

int main() {
    std::string sql = "CREATE TABLE users (id LONG)";
    std::cout << "SQL: " << sql << "\n\n";
    
    try {
        Lexer lexer(sql);
        std::vector<Token> tokens = lexer.tokenize();
        
        std::cout << "Tokens:\n";
        for (size_t i = 0; i < tokens.size(); ++i) {
            std::cout << i << ": type=" << static_cast<int>(tokens[i].type) 
                      << " val='" << tokens[i].value << "'\n";
        }
        std::cout << "\nParsing...\n";
        
        Parser parser(tokens);
        SQLCommand cmd = parser.parse();
        
        auto* create = std::get_if<std::unique_ptr<CreateTableStmt>>(&cmd);
        if (create) {
            std::cout << "Success! Table: " << (*create)->tableName << "\n";
        }
    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << "\n";
    }
    
    return 0;
}
