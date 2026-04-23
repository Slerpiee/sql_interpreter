#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <stdexcept>

namespace sql {

// Типы токенов
enum class TokenType {
    // Ключевые слова
    KEYWORD_SELECT,
    KEYWORD_FROM,
    KEYWORD_WHERE,
    KEYWORD_INSERT,
    KEYWORD_INTO,
    KEYWORD_UPDATE,
    KEYWORD_SET,
    KEYWORD_DELETE,
    KEYWORD_CREATE,
    KEYWORD_TABLE,
    KEYWORD_DROP,
    KEYWORD_TEXT,
    KEYWORD_LONG,
    KEYWORD_LIKE,
    KEYWORD_IN,
    KEYWORD_AND,
    KEYWORD_OR,
    KEYWORD_NOT,
    KEYWORD_ALL,
    
    // Литералы
    INTEGER_LITERAL,      // целое число
    STRING_LITERAL,       // строка в одинарных кавычках
    IDENTIFIER,           // имя (идентификатор)
    
    // Операторы и пунктуация
    PLUS,                 // +
    MINUS,                // -
    STAR,                 // *
    SLASH,                // /
    PERCENT,              // %
    EQUAL,                // =
    NOT_EQUAL,            // !=
    LESS,                 // <
    LESS_EQUAL,           // <=
    GREATER,              // >
    GREATER_EQUAL,        // >=
    COMMA,                // ,
    LPAREN,               // (
    RPAREN,               // )
    SEMICOLON,            // ; (опционально)
    
    // Специальные
    EOF_TOKEN,            // конец входных данных
    INVALID               // недопустимый токен
};

// Структура токена
struct Token {
    TokenType type;
    std::string value;    // текстовое представление (для идентификаторов, литералов)
    size_t line;          // номер строки
    size_t column;        // позиция в строке
    
    Token(TokenType t = TokenType::INVALID, 
          const std::string& v = "", 
          size_t l = 1, 
          size_t c = 1)
        : type(t), value(v), line(l), column(c) {}
};

// Исключение для лексических ошибок
class LexerException : public std::runtime_error {
public:
    size_t line;
    size_t column;
    
    LexerException(const std::string& msg, size_t l, size_t c)
        : std::runtime_error(msg + " at line " + std::to_string(l) + ", column " + std::to_string(c)),
          line(l), column(c) {}
};

// Лексический анализатор
class Lexer {
public:
    explicit Lexer(const std::string& input);
    
    // Получить следующий токен
    Token nextToken();
    
    // Посмотреть следующий токен без продвижения
    Token peek();
    
    // Вернуть токен обратно (для одного уровня lookahead)
    void putBack(const Token& token);
    
private:
    std::string input;
    size_t pos;           // текущая позиция
    size_t line;          // текущая строка
    size_t column;        // текущая колонка
    bool hasPutBack;      // есть ли отложенный токен
    Token putBackToken;   // отложенный токен
    
    // Пропустить пробельные символы и комментарии
    void skipWhitespace();
    
    // Прочитать идентификатор или ключевое слово
    Token readIdentifier();
    
    // Прочитать число
    Token readNumber();
    
    // Прочитать строку
    Token readString();
    
    // Проверить, является ли символ буквой
    bool isAlpha(char c) const;
    
    // Проверить, является ли символ цифрой
    bool isDigit(char c) const;
    
    // Проверить, является ли символ частью идентификатора
    bool isIdentChar(char c) const;
    
    // Получить текущий символ
    char currentChar() const;
    
    // Продвинуться вперед
    void advance();
};

} // namespace sql

#endif // LEXER_H
