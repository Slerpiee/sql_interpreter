#include <iostream>
#include <string>
#include <vector>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

// Тестовые SQL-запросы для демонстрации работы парсера
const std::vector<std::string> testQueries = {
    // CREATE TABLE
    "CREATE TABLE users (id LONG, name TEXT(50), email TEXT(100))",
    
    // DROP TABLE
    "DROP TABLE users",
    
    // SELECT с WHERE ALL
    "SELECT * FROM users WHERE ALL",
    
    // SELECT со списком полей
    "SELECT id, name, email FROM users",
    
    // SELECT с простым WHERE (сравнение)
    "SELECT * FROM users WHERE id = 5",
    
    // SELECT с текстовым сравнением
    "SELECT name FROM users WHERE name = 'John'",
    
    // SELECT с арифметическим выражением
    "SELECT * FROM products WHERE price > 100 + 50",
    
    // SELECT со сложным условием (AND/OR)
    "SELECT * FROM users WHERE age >= 18 AND status = 'active'",
    
    // SELECT с NOT
    "SELECT * FROM users WHERE NOT age < 18",
    
    // INSERT
    "INSERT INTO users ('John Doe', 25)",
    
    // INSERT с несколькими значениями
    "INSERT INTO products ('Widget', 10, 100)",
    
    // UPDATE
    "UPDATE users SET name = 'Jane' WHERE id = 1",
    
    // UPDATE с арифметикой
    "UPDATE products SET price = price * 1.1 WHERE category = 'electronics'",
    
    // DELETE
    "DELETE FROM users WHERE id = 5",
    
    // DELETE с условием
    "DELETE FROM logs WHERE timestamp < 1000000",
    
    // Сложное логическое выражение
    "SELECT * FROM orders WHERE (status = 'pending' OR status = 'processing') AND total > 100",
    
    // Сравнение с != 
    "SELECT * FROM users WHERE status != 'deleted'",
    
    // SELECT без WHERE
    "SELECT id, name FROM categories"
};

void runTest(const std::string& query) {
    std::cout << "\n========================================\n";
    std::cout << "Query: " << query << "\n";
    std::cout << "========================================\n\n";
    
    try {
        sql::Lexer lexer(query);
        sql::Parser parser(lexer);
        
        auto ast = parser.parse();
        
        std::cout << "AST:\n";
        std::cout << ast->ToString(0) << "\n";
        std::cout << "\n[SUCCESS] Parsed successfully!\n";
        
    } catch (const sql::LexerException& e) {
        std::cerr << "[LEXER ERROR] " << e.what() << "\n";
    } catch (const sql::ParseException& e) {
        std::cerr << "[PARSER ERROR] " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=====================================================\n";
    std::cout << "       SQL Parser Demo (Recursive Descent)          \n";
    std::cout << "=====================================================\n";
    
    if (argc > 1) {
        // Если передан аргумент командной строки - парсим его
        std::string query;
        for (int i = 1; i < argc; ++i) {
            if (i > 1) query += " ";
            query += argv[i];
        }
        runTest(query);
    } else {
        // Иначе запускаем все тестовые запросы
        std::cout << "\nRunning " << testQueries.size() << " test queries...\n";
        
        for (const auto& query : testQueries) {
            runTest(query);
        }
        
        // Интерактивный режим
        std::cout << "\n========================================\n";
        std::cout << "Interactive mode (type 'quit' to exit):\n";
        std::cout << "========================================\n\n";
        
        std::string line;
        while (true) {
            std::cout << "SQL> ";
            if (!std::getline(std::cin, line)) {
                break;
            }
            
            // Пропускаем пустые строки
            if (line.empty()) {
                continue;
            }
            
            // Проверка на выход
            if (line == "quit" || line == "exit") {
                std::cout << "Goodbye!\n";
                break;
            }
            
            runTest(line);
        }
    }
    
    return 0;
}
