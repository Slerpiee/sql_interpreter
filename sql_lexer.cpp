#include "sql_lexer.h"
#include <cctype>
#include <sstream>
#include <unordered_map>

namespace SQL {

// Карта ключевых слов
static const std::unordered_map<std::string, TokenType> keywords = {
    {"SELECT", TokenType::SELECT},
    {"INSERT", TokenType::INSERT},
    {"UPDATE", TokenType::UPDATE},
    {"DELETE", TokenType::DELETE},
    {"FROM", TokenType::FROM},
    {"WHERE", TokenType::WHERE},
    {"SET", TokenType::SET},
    {"INTO", TokenType::INTO},
    {"VALUES", TokenType::VALUES},
    {"CREATE", TokenType::CREATE},
    {"TABLE", TokenType::TABLE},
    {"DROP", TokenType::DROP},
    {"AND", TokenType::AND},
    {"OR", TokenType::OR},
    {"NOT", TokenType::NOT}
};

// Лексер - разбивает входную строку на токены
Lexer::Lexer(const std::string& input)
    : input(input), currentPos(0), currentLine(1), currentColumn(1) {}

// Получить текущий символ
char Lexer::currentChar() const {
    if (currentPos >= input.length()) {
        return '\0';
    }
    return input[currentPos];
}

// Перейти к следующему символу
void Lexer::advance() {
    if (currentPos < input.length()) {
        if (input[currentPos] == '\n') {
            currentLine++;
            currentColumn = 1;
        } else {
            currentColumn++;
        }
        currentPos++;
    }
}

// Проверить, является ли символ буквой
bool Lexer::isAlpha(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

// Проверить, является ли символ цифрой
bool Lexer::isDigit(char c) {
    return std::isdigit(static_cast<unsigned char>(c));
}

// Проверить, является ли символ alphanumeric
bool Lexer::isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

// Пропустить пробельные символы и комментарии
void Lexer::skipWhitespace() {
    while (true) {
        char c = currentChar();
        
        // Пропускаем пробелы, табуляции, переводы строк
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance();
            continue;
        }
        
        // Пропускаем однострочные комментарии --
        if (c == '-' && currentPos + 1 < input.length() && input[currentPos + 1] == '-') {
            while (currentChar() != '\n' && !isEnd()) {
                advance();
            }
            continue;
        }
        
        // Пропускаем многострочные комментарии /* */
        if (c == '/' && currentPos + 1 < input.length() && input[currentPos + 1] == '*') {
            advance(); // пропускаем /
            advance(); // пропускаем *
            while (!isEnd()) {
                if (currentChar() == '*' && currentPos + 1 < input.length() && input[currentPos + 1] == '/') {
                    advance(); // пропускаем *
                    advance(); // пропускаем /
                    break;
                }
                advance();
            }
            continue;
        }
        
        break;
    }
}

// Считать идентификатор или ключевое слово
Token Lexer::readIdentifier() {
    int startLine = currentLine;
    int startCol = currentColumn;
    std::string value;
    
    while (isAlphaNumeric(currentChar())) {
        value += currentChar();
        advance();
    }
    
    // Преобразуем в верхний регистр для поиска ключевых слов
    std::string upperValue = value;
    for (char& c : upperValue) {
        c = std::toupper(static_cast<unsigned char>(c));
    }
    
    // Проверяем, является ли ключевым словом
    auto it = keywords.find(upperValue);
    if (it != keywords.end()) {
        return Token(it->second, value, startLine, startCol);
    }
    
    return Token(TokenType::IDENTIFIER, value, startLine, startCol);
}

// Считать числовой литерал
Token Lexer::readNumber() {
    int startLine = currentLine;
    int startCol = currentColumn;
    std::string value;
    
    while (isDigit(currentChar())) {
        value += currentChar();
        advance();
    }
    
    return Token(TokenType::INTEGER_LITERAL, value, startLine, startCol);
}

// Считать строковый литерал
Token Lexer::readString() {
    int startLine = currentLine;
    int startCol = currentColumn;
    std::string value;
    
    advance(); // пропускаем открывающую кавычку
    
    while (currentChar() != '\'' && !isEnd()) {
        if (currentChar() == '\\' && currentPos + 1 < input.length()) {
            advance(); // пропускаем backslash
            if (!isEnd()) {
                value += currentChar();
                advance();
            }
        } else {
            value += currentChar();
            advance();
        }
    }
    
    if (currentChar() != '\'') {
        throw LexerException("Unterminated string literal", startLine, startCol);
    }
    
    advance(); // пропускаем закрывающую кавычку
    
    return Token(TokenType::STRING_LITERAL, value, startLine, startCol);
}

// Создать токен с текущей позицией
Token Lexer::makeToken(TokenType type, const std::string& value) {
    return Token(type, value.empty() ? std::string(1, currentChar()) : value, 
                 currentLine, currentColumn);
}

// Получить следующий токен
Token Lexer::nextToken() {
    skipWhitespace();
    
    if (isEnd()) {
        return Token(TokenType::END_OF_INPUT, "", currentLine, currentColumn);
    }
    
    int startLine = currentLine;
    int startCol = currentColumn;
    char c = currentChar();
    
    // Идентификаторы и ключевые слова
    if (isAlpha(c)) {
        return readIdentifier();
    }
    
    // Числа
    if (isDigit(c)) {
        return readNumber();
    }
    
    // Строки
    if (c == '\'') {
        return readString();
    }
    
    // Операторы и разделители
    advance();
    
    switch (c) {
        case '=':
            return Token(TokenType::EQUALS, "=", startLine, startCol);
        
        case '<':
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::LESS_EQ, "<=", startLine, startCol);
            }
            if (currentChar() == '>') {
                advance();
                return Token(TokenType::NOT_EQUALS, "<>", startLine, startCol);
            }
            return Token(TokenType::LESS, "<", startLine, startCol);
        
        case '>':
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::GREATER_EQ, ">=", startLine, startCol);
            }
            return Token(TokenType::GREATER, ">", startLine, startCol);
        
        case '!':
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::NOT_EQUALS, "!=", startLine, startCol);
            }
            throw LexerException("Unexpected character '!'", startLine, startCol);
        
        case ',':
            return Token(TokenType::COMMA, ",", startLine, startCol);
        
        case ';':
            return Token(TokenType::SEMICOLON, ";", startLine, startCol);
        
        case '(':
            return Token(TokenType::LPAREN, "(", startLine, startCol);
        
        case ')':
            return Token(TokenType::RPAREN, ")", startLine, startCol);
        
        case '*':
            return Token(TokenType::STAR, "*", startLine, startCol);
        
        case '.':
            return Token(TokenType::DOT, ".", startLine, startCol);
        
        default:
            throw LexerException("Unexpected character '" + std::string(1, c) + "'", 
                               startLine, startCol);
    }
}

// Получить все токены
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (!isEnd()) {
        Token token = nextToken();
        tokens.push_back(token);
        if (token.type == TokenType::END_OF_INPUT) {
            break;
        }
    }
    
    return tokens;
}

}
