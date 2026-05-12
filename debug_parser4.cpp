#include <iostream>
#include "sql_lexer.h"
#include "sql_parser.h"

using namespace SQL;

int main() {
    std::string sql = "CREATE TABLE users (id LONG)";
    std::cout << "SQL: " << sql << "\n\n";
    
    Lexer lexer(sql);
    auto tokens = lexer.tokenize();
    
    std::cout << "Tokens:\n";
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        std::cout << "[" << i << "] type=" << static_cast<int>(token.type) << " | '" << token.value << "'\n";
    }
    
    // Simulate parseCreateTable manually
    std::cout << "\nSimulating parseCreateTable:\n";
    Parser parser(tokens);
    
    // Step 1: advance() - skip CREATE
    std::cout << "Step 1: advance() - skip CREATE\n";
    // Can't call advance directly, it's private
    
    // Let's trace through parseColumnDefs instead
    std::cout << "\nThe issue is in parseColumnDefs - it expects IDENTIFIER for column name,\n";
    std::cout << "but after '(' we have 'id' which IS an IDENTIFIER.\n";
    std::cout << "Then it advances and expects IDENTIFIER or LPAREN for type.\n";
    std::cout << "After 'id', current token is 'LONG' which is IDENTIFIER - OK.\n";
    std::cout << "Then advance(), now at ')'.\n";
    std::cout << "Check for TEXT and LPAREN - no, typeName='LONG'.\n";
    std::cout << "Add column, check for COMMA - no, it's RPAREN.\n";
    std::cout << "Break from do-while.\n";
    std::cout << "Return to parseCreateTable.\n";
    std::cout << "Now expect RPAREN - should work!\n";
    
    return 0;
}
