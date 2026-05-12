#include <iostream>
#include <iomanip>
#include "sql_executor.h"

using namespace SQL;

// Функция для печати результата запроса
void printResult(const QueryResult& result) {
    if (!result.success) {
        std::cout << "ERROR: " << result.message << "\n";
        return;
    }
    
    std::cout << result.message << "\n";
    
    // Печать заголовков
    if (!result.columnNames.empty()) {
        std::cout << "\n";
        for (size_t i = 0; i < result.columnNames.size(); ++i) {
            std::cout << std::left << std::setw(20) << result.columnNames[i];
        }
        std::cout << "\n";
        std::cout << std::string(result.columnNames.size() * 20, '-') << "\n";
        
        // Печать строк
        for (const auto& row : result.rows) {
            for (size_t i = 0; i < row.size() && i < result.columnNames.size(); ++i) {
                std::cout << std::left << std::setw(20) << row[i];
            }
            std::cout << "\n";
        }
    }
}

void testExecutor(Executor& executor, const std::string& sql, const std::string& description) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "TEST: " << description << "\n";
    std::cout << "SQL:  " << sql << "\n";
    std::cout << std::string(60, '=') << "\n";
    
    QueryResult result = executor.execute(sql);
    printResult(result);
}

int main() {
    std::cout << "SQL EXECUTOR TEST\n";
    std::cout << "=================\n";
    
    Executor executor;
    
    // Тест 1: Создание таблицы
    testExecutor(executor, 
        "CREATE TABLE users (id LONG, name TEXT(50), age LONG, email TEXT(100))",
        "CREATE TABLE users");
    
    // Тест 2: Вставка данных
    testExecutor(executor,
        "INSERT INTO users VALUES (1, 'Alice', 25, 'alice@example.com')",
        "INSERT first user");
    
    testExecutor(executor,
        "INSERT INTO users VALUES (2, 'Bob', 30, 'bob@test.com')",
        "INSERT second user");
    
    testExecutor(executor,
        "INSERT INTO users VALUES (3, 'Charlie', 35, 'charlie@mail.com')",
        "INSERT third user");
    
    // Тест 3: SELECT все записи
    testExecutor(executor,
        "SELECT * FROM users",
        "SELECT all users");
    
    // Тест 4: SELECT с WHERE
    testExecutor(executor,
        "SELECT name, age FROM users WHERE age > 25",
        "SELECT users older than 25");
    
    // Тест 5: UPDATE
    testExecutor(executor,
        "UPDATE users SET age = 26 WHERE name = 'Alice'",
        "UPDATE Alice's age");
    
    // Проверка обновления
    testExecutor(executor,
        "SELECT * FROM users WHERE name = 'Alice'",
        "Verify Alice's update");
    
    // Тест 6: SELECT с несколькими условиями
    testExecutor(executor,
        "SELECT name, email FROM users WHERE age >= 30 AND id < 3",
        "SELECT with multiple conditions");
    
    // Тест 7: INSERT с указанием колонок
    testExecutor(executor,
        "INSERT INTO users (id, name, age, email) VALUES (4, 'Diana', 28, 'diana@example.com')",
        "INSERT with column list");
    
    // Тест 8: Проверка всех записей
    testExecutor(executor,
        "SELECT * FROM users",
        "SELECT all users after inserts");
    
    // Тест 9: Семантическая ошибка - несуществующая таблица
    testExecutor(executor,
        "SELECT * FROM nonexistent_table",
        "ERROR: Non-existent table");
    
    // Тест 10: Семантическая ошибка - несуществующая колонка
    testExecutor(executor,
        "SELECT invalid_column FROM users",
        "ERROR: Non-existent column");
    
    // Тест 11: Попытка создать таблицу повторно
    testExecutor(executor,
        "CREATE TABLE users (id LONG)",
        "ERROR: Duplicate table creation");
    
    // Тест 12: DROP TABLE
    testExecutor(executor,
        "DROP TABLE users",
        "DROP TABLE users");
    
    // Тест 13: Проверка что таблица удалена
    testExecutor(executor,
        "SELECT * FROM users",
        "ERROR: Access dropped table");
    
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "ALL EXECUTOR TESTS COMPLETED\n";
    std::cout << std::string(60, '=') << "\n";
    
    return 0;
}
