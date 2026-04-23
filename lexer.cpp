#include "lexer.h"
#include <cctype>
#include <unordered_map>

namespace sql {

// Таблица ключевых слов
static const std::unordered_map<std::string, TokenType> keywords = {
    {"SELECT", TokenType::KEYWORD_SELECT},
    {"FROM", TokenType::KEYWORD_FROM},
    {"WHERE", TokenType::KEYWORD_WHERE},
    {"INSERT", TokenType::KEYWORD_INSERT},
    {"INTO", TokenType::KEYWORD_INTO},
    {"UPDATE", TokenType::KEYWORD_UPDATE},
    {"SET", TokenType::KEYWORD_SET},
    {"DELETE", TokenType::KEYWORD_DELETE},
    {"CREATE", TokenType::KEYWORD_CREATE},
    {"TABLE", TokenType::KEYWORD_TABLE},
    {"DROP", TokenType::KEYWORD_DROP},
    {"TEXT", TokenType::KEYWORD_TEXT},
    {"LONG", TokenType::KEYWORD_LONG},
    {"LIKE", TokenType::KEYWORD_LIKE},
    {"IN", TokenType::KEYWORD_IN},
    {"AND", TokenType::KEYWORD_AND},
    {"OR", TokenType::KEYWORD_OR},
    {"NOT", TokenType::KEYWORD_NOT},
    {"ALL", TokenType::KEYWORD_ALL}
};

Lexer::Lexer(const std::string& input)
    : input(input), pos(0), line(1), column(1), 
      hasPutBack(false), putBackToken() {}

bool Lexer::isAlpha(char c) const {
    return std::isalpha(static_cast<unsigned char>(c)) != 0;
}

bool Lexer::isDigit(char c) const {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

bool Lexer::isIdentChar(char c) const {
    return isAlpha(c) || isDigit(c) || c == '_';
}

char Lexer::currentChar() const {
    if (pos >= input.size()) {
        return '\0';
    }
    return input[pos];
}

void Lexer::advance() {
    if (pos < input.size()) {
        if (input[pos] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        pos++;
    }
}

void Lexer::skipWhitespace() {
    while (true) {
        char c = currentChar();
        if (c == '\0') break;
        
        // Пропускаем пробельные символы
        if (std::isspace(static_cast<unsigned char>(c))) {
            advance();
            continue;
        }
        
        // Пропускаем однострочные комментарии (-- ...)
        if (c == '-' && pos + 1 < input.size() && input[pos + 1] == '-') {
            while (currentChar() != '\n' && currentChar() != '\0') {
                advance();
            }
            continue;
        }
        
        // Пропускаем многострочные комментарии (/* ... */)
        if (c == '/' && pos + 1 < input.size() && input[pos + 1] == '*') {
            advance(); // пропускаем '/'
            advance(); // пропускаем '*'
            while (true) {
                if (currentChar() == '\0') break;
                if (currentChar() == '*' && pos + 1 < input.size() && input[pos + 1] == '/') {
                    advance(); // пропускаем '*'
                    advance(); // пропускаем '/'
                    break;
                }
                advance();
            }
            continue;
        }
        
        break;
    }
}

Token Lexer::readIdentifier() {
    size_t startLine = line;
    size_t startCol = column;
    std::string value;
    
    while (isIdentChar(currentChar())) {
        value += currentChar();
        advance();
    }
    
    // Проверяем, является ли это ключевым словом
    std::string upperValue = value;
    for (char& c : upperValue) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    
    auto it = keywords.find(upperValue);
    if (it != keywords.end()) {
        return Token(it->second, value, startLine, startCol);
    }
    
    return Token(TokenType::IDENTIFIER, value, startLine, startCol);
}

Token Lexer::readNumber() {
    size_t startLine = line;
    size_t startCol = column;
    std::string value;
    
    // Опциональный знак
    if (currentChar() == '+' || currentChar() == '-') {
        value += currentChar();
        advance();
    }
    
    // Цифры
    while (isDigit(currentChar())) {
        value += currentChar();
        advance();
    }
    
    return Token(TokenType::INTEGER_LITERAL, value, startLine, startCol);
}

Token Lexer::readString() {
    size_t startLine = line;
    size_t startCol = column;
    std::string value;
    
    // Пропускаем открывающую кавычку
    advance();
    
    while (currentChar() != '\'' && currentChar() != '\0') {
        value += currentChar();
        advance();
    }
    
    if (currentChar() == '\0') {
        throw LexerException("Unterminated string literal", startLine, startCol);
    }
    
    // Пропускаем закрывающую кавычку
    advance();
    
    return Token(TokenType::STRING_LITERAL, value, startLine, startCol);
}

Token Lexer::nextToken() {
    // Возвращаем отложенный токен, если есть
    if (hasPutBack) {
        hasPutBack = false;
        return putBackToken;
    }
    
    skipWhitespace();
    
    size_t startLine = line;
    size_t startCol = column;
    char c = currentChar();
    
    if (c == '\0') {
        return Token(TokenType::EOF_TOKEN, "", startLine, startCol);
    }
    
    // Идентификатор или ключевое слово
    if (isAlpha(c)) {
        return readIdentifier();
    }
    
    // Число
    if (isDigit(c) || (c == '-' && pos + 1 < input.size() && isDigit(input[pos + 1]))) {
        return readNumber();
    }
    
    // Строка
    if (c == '\'') {
        return readString();
    }
    
    // Операторы и пунктуация
    advance();
    
    switch (c) {
        case '+':
            return Token(TokenType::PLUS, "+", startLine, startCol);
        case '-':
            return Token(TokenType::MINUS, "-", startLine, startCol);
        case '*':
            return Token(TokenType::STAR, "*", startLine, startCol);
        case '/':
            return Token(TokenType::SLASH, "/", startLine, startCol);
        case '%':
            return Token(TokenType::PERCENT, "%", startLine, startCol);
        case ',':
            return Token(TokenType::COMMA, ",", startLine, startCol);
        case '(':
            return Token(TokenType::LPAREN, "(", startLine, startCol);
        case ')':
            return Token(TokenType::RPAREN, ")", startLine, startCol);
        case ';':
            return Token(TokenType::SEMICOLON, ";", startLine, startCol);
        case '=':
            return Token(TokenType::EQUAL, "=", startLine, startCol);
        case '!':
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::NOT_EQUAL, "!=", startLine, startCol);
            }
            throw LexerException("Unexpected character '!'", startLine, startCol);
        case '<':
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::LESS_EQUAL, "<=", startLine, startCol);
            }
            return Token(TokenType::LESS, "<", startLine, startCol);
        case '>':
            if (currentChar() == '=') {
                advance();
                return Token(TokenType::GREATER_EQUAL, ">=", startLine, startCol);
            }
            return Token(TokenType::GREATER, ">", startLine, startCol);
        default:
            throw LexerException("Unexpected character '" + std::string(1, c) + "'", 
                                startLine, startCol);
    }
}

Token Lexer::peek() {
    if (!hasPutBack) {
        putBackToken = nextToken();
        hasPutBack = true;
    }
    return putBackToken;
}

void Lexer::putBack(const Token& token) {
    if (hasPutBack) {
        throw std::runtime_error("Cannot put back more than one token");
    }
    putBackToken = token;
    hasPutBack = true;
}

} // namespace sql
