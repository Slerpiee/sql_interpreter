#ifndef SQL_TOKENS_H
#define SQL_TOKENS_H

#include <string>

namespace SQL {

// Типы токенов
enum class TokenType {
    // Ключевые слова
    SELECT,
    INSERT,
    UPDATE,
    DELETE,
    FROM,
    WHERE,
    SET,
    INTO,
    VALUES,
    CREATE,
    TABLE,
    DROP,
    
    // Идентификаторы и литералы
    IDENTIFIER,
    STRING_LITERAL,
    INTEGER_LITERAL,
    
    // Операторы
    EQUALS,        // =
    NOT_EQUALS,    // != или <>
    LESS,          // <
    GREATER,       // >
    LESS_EQ,       // <=
    GREATER_EQ,    // >=
    AND,
    OR,
    NOT,
    
    // Разделители
    COMMA,         // ,
    SEMICOLON,     // ;
    LPAREN,        // (
    RPAREN,        // )
    STAR,          // *
    DOT,           // .
    
    // Специальные
    END_OF_INPUT,
    UNKNOWN
};

// Структура токена
struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token(TokenType t = TokenType::UNKNOWN, const std::string& v = "", int l = 0, int c = 0)
        : type(t), value(v), line(l), column(c) {}
};

// Функция для получения строкового представления типа токена
const char* tokenTypeToString(TokenType type);

}

#endif // SQL_TOKENS_H
