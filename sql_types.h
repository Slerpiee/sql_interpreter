#ifndef SQL_TYPES_H
#define SQL_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>

// Типы данных SQL
enum class SQLType {
    INTEGER,
    VARCHAR,
    UNKNOWN
};

// Вариант значения ячейки
using SQLValue = std::variant<int, std::string, std::nullptr_t>;

// Структура для хранения результата запроса
struct SQLResult {
    std::vector<std::string> columns;
    std::vector<std::map<std::string, SQLValue>> rows;
    bool success;
    std::string errorMessage;
    
    SQLResult() : success(true) {}
    
    static SQLResult error(const std::string& msg) {
        SQLResult result;
        result.success = false;
        result.errorMessage = msg;
        return result;
    }
};

// Структура для описания поля таблицы
struct FieldDefinition {
    std::string name;
    SQLType type;
    size_t length;
    
    FieldDefinition() : type(SQLType::INTEGER), length(0) {}
    FieldDefinition(const std::string& n, SQLType t, size_t l = 0) 
        : name(n), type(t), length(l) {}
};

#endif // SQL_TYPES_H
