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
        for (const auto& token : tokens) {
            std::cout << "  " << tokenTypeToString(token.type) << " | '" << token.value << "'\n";
        }
        
        std::cout << "\nParsing...\n";
        Parser parser(tokens);
        auto cmd = parser.parse();
        
        if (auto* create = std::get_if<std::unique_ptr<CreateTableStmt>>(&cmd)) {
            std::cout << "Successfully parsed CREATE TABLE!\n";
            std::cout << "Table name: " << (*create)->tableName << "\n";
            std::cout << "Columns: " << (*create)->columns.size() << "\n";
            for (const auto& col : (*create)->columns) {
                std::cout << "  - " << col.name << " " << col.type;
                if (col.size > 0) std::cout << "(" << col.size << ")";
                std::cout << "\n";
            }
        }
    } catch (const ParserException& e) {
        std::cout << "Parser error: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    
    return 0;
}
