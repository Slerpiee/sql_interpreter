#include <iostream>
#include <iomanip>
#include "sql_lexer.h"
#include "sql_parser.h"

using namespace SQL;

// Функция для печати токенов
void printTokens(const std::vector<Token>& tokens) {
    std::cout << "\n=== TOKENS ===\n";
    std::cout << std::left << std::setw(6) << "Line" 
              << std::setw(8) << "Column" 
              << std::setw(20) << "Type" 
              << "Value\n";
    std::cout << std::string(50, '-') << "\n";
    
    for (const auto& token : tokens) {
        if (token.type == TokenType::END_OF_INPUT) break;
        
        std::cout << std::left << std::setw(6) << token.line
                  << std::setw(8) << token.column
                  << std::setw(20) << tokenTypeToString(token.type);
        
        // Экранируем специальные символы в значении
        std::string displayValue = token.value;
        if (token.type == TokenType::STRING_LITERAL) {
            displayValue = "'" + displayValue + "'";
        }
        std::cout << displayValue << "\n";
    }
}

// Функция для печати AST
void printAST(const SQLCommand& cmd) {
    std::cout << "\n=== PARSED COMMAND ===\n";
    
    std::visit([](const auto& stmt) {
        std::cout << stmt->toString() << "\n";
    }, cmd);
}

// Тестирование различных SQL команд
void testSQL(const std::string& sql, const std::string& description) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "TEST: " << description << "\n";
    std::cout << "SQL:  " << sql << "\n";
    std::cout << std::string(60, '=') << "\n";
    
    try {
        // Лексический анализ
        Lexer lexer(sql);
        std::vector<Token> tokens = lexer.tokenize();
        printTokens(tokens);
        
        // Синтаксический анализ
        Parser parser(tokens);
        SQLCommand cmd = parser.parse();
        printAST(cmd);
        
        std::cout << "\nSUCCESS\n";
    } catch (const LexerException& e) {
        std::cout << "\nLEXER ERROR at line " << e.getLine() 
                  << ", column " << e.getColumn() << ": " << e.what() << "\n";
    } catch (const ParserException& e) {
        const Token& t = e.getToken();
        std::cout << "\nPARSER ERROR at line " << t.line 
                  << ", column " << t.column << ": " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cout << "\nERROR: " << e.what() << "\n";
    }
}

int main() {
    std::cout << "SQL LEXER AND PARSER TEST\n";
    std::cout << "=========================\n";
    
    // Тест 1: SELECT с WHERE
    testSQL(
        "SELECT id, name, age FROM users WHERE age > 18",
        "Simple SELECT with WHERE clause"
    );
    
    // Тест 2: SELECT с несколькими условиями
    testSQL(
        "SELECT * FROM products WHERE price >= 100 AND stock > 0",
        "SELECT with multiple conditions"
    );
    
    // Тест 3: INSERT
    testSQL(
        "INSERT INTO users (id, name, email) VALUES (1, 'John', 'john@example.com')",
        "INSERT with columns"
    );
    
    // Тест 4: INSERT без указания колонок
    testSQL(
        "INSERT INTO users VALUES (2, 'Jane', 'jane@test.com')",
        "INSERT without column list"
    );
    
    // Тест 5: UPDATE
    testSQL(
        "UPDATE users SET name = 'Bob', age = 25 WHERE id = 1",
        "UPDATE with multiple assignments"
    );
    
    // Тест 6: DELETE
    testSQL(
        "DELETE FROM users WHERE age < 18 OR status = 'inactive'",
        "DELETE with OR condition"
    );
    
    // Тест 7: CREATE TABLE
    testSQL(
        "CREATE TABLE users (id LONG, name TEXT(50), email TEXT(100))",
        "CREATE TABLE with different types"
    );
    
    // Тест 8: DROP TABLE
    testSQL(
        "DROP TABLE old_table",
        "DROP TABLE"
    );
    
    // Тест 9: SELECT с таблицей.колонка
    testSQL(
        "SELECT u.id, u.name FROM users u WHERE u.age > 21",
        "SELECT with table aliases"
    );
    
    // Тест 10: Комментарии
    testSQL(
        "-- Это комментарий\nSELECT id FROM test /* многострочный */ WHERE x = 5",
        "SQL with comments"
    );
    
    // Тест 11: Ошибка - некорректный синтаксис
    testSQL(
        "SELECT FROM users",
        "ERROR: Missing column list"
    );
    
    // Тест 12: Ошибка - неизвестный токен
    testSQL(
        "SELECT @invalid FROM users",
        "ERROR: Invalid character"
    );
    
    // Тест 13: NULL значение
    testSQL(
        "INSERT INTO users VALUES (3, NULL, 'test@test.com')",
        "INSERT with NULL value"
    );
    
    // Тест 14: Разные операторы сравнения
    testSQL(
        "SELECT * FROM items WHERE count != 0 AND price <> 100",
        "Different comparison operators (!= and <>)"
    );
    
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "ALL TESTS COMPLETED\n";
    std::cout << std::string(60, '=') << "\n";

    
    return 0;
}
