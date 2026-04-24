#ifndef SQL_LEXER_H
#define SQL_LEXER_H

#include "sql_tokens.h"
#include <string>
#include <vector>
#include <stdexcept>

namespace SQL {

// Исключение для лексических ошибок
class LexerException : public std::runtime_error {
public:
    LexerException(const std::string& msg, int line, int column)
        : std::runtime_error(msg), line(line), column(column) {}
    
    int getLine() const { return line; }
    int getColumn() const { return column; }
    
private:
    int line;
    int column;
};

// Лексер - разбивает входную строку на токены
class Lexer {
public:
    explicit Lexer(const std::string& input);
    
    // Получить следующий токен
    Token nextToken();
    
    // Получить все токены
    std::vector<Token> tokenize();
    
    // Проверить, достигнут ли конец ввода
    bool isEnd() const { return currentPos >= input.length(); }
    
private:
    std::string input;
    size_t currentPos;
    int currentLine;
    int currentColumn;
    
    // Пропустить пробельные символы и комментарии
    void skipWhitespace();
    
    // Считать идентификатор или ключевое слово
    Token readIdentifier();
    
    // Считать числовой литерал
    Token readNumber();
    
    // Считать строковый литерал
    Token readString();
    
    // Создать токен с текущей позицией
    Token makeToken(TokenType type, const std::string& value = "");
    
    // Получить текущий символ
    char currentChar() const;
    
    // Перейти к следующему символу
    void advance();
    
    // Проверить, является ли символ буквой
    static bool isAlpha(char c);
    
    // Проверить, является ли символ цифрой
    static bool isDigit(char c);
    
    // Проверить, является ли символ alphanumeric
    static bool isAlphaNumeric(char c);
};

}

#endif
