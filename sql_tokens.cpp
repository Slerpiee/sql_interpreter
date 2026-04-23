#include "sql_tokens.h"

namespace SQL {

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::SELECT: return "SELECT";
        case TokenType::INSERT: return "INSERT";
        case TokenType::UPDATE: return "UPDATE";
        case TokenType::DELETE: return "DELETE";
        case TokenType::FROM: return "FROM";
        case TokenType::WHERE: return "WHERE";
        case TokenType::SET: return "SET";
        case TokenType::INTO: return "INTO";
        case TokenType::VALUES: return "VALUES";
        case TokenType::CREATE: return "CREATE";
        case TokenType::TABLE: return "TABLE";
        case TokenType::DROP: return "DROP";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::EQUALS: return "=";
        case TokenType::NOT_EQUALS: return "!=";
        case TokenType::LESS: return "<";
        case TokenType::GREATER: return ">";
        case TokenType::LESS_EQ: return "<=";
        case TokenType::GREATER_EQ: return ">=";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::COMMA: return ",";
        case TokenType::SEMICOLON: return ";";
        case TokenType::LPAREN: return "(";
        case TokenType::RPAREN: return ")";
        case TokenType::STAR: return "*";
        case TokenType::DOT: return ".";
        case TokenType::END_OF_INPUT: return "END_OF_INPUT";
        case TokenType::UNKNOWN: return "UNKNOWN";
        default: return "INVALID";
    }
}

} // namespace SQL
