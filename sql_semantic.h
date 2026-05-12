#ifndef SQL_SEMANTIC_H
#define SQL_SEMANTIC_H

#include "sql_parser.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stdexcept>

namespace SQL {

// Исключение для семантических ошибок
class SemanticException : public std::runtime_error {
public:
    explicit SemanticException(const std::string& msg) 
        : std::runtime_error(msg) {}
};

// Информация о поле таблицы
struct FieldInfo {
    std::string name;
    std::string type;      // "LONG" или "TEXT"
    long size;             // размер для TEXT(n)
};

// Информация о таблице
struct TableInfo {
    std::string name;
    std::vector<FieldInfo> fields;
    
    // Найти поле по имени
    const FieldInfo* findField(const std::string& fieldName) const;
};

// Символьная таблица - хранит информацию о всех таблицах
class SymbolTable {
public:
    // Зарегистрировать таблицу
    void addTable(const std::string& tableName, const std::vector<FieldInfo>& fields);
    
    // Удалить таблицу
    void removeTable(const std::string& tableName);
    
    // Получить информацию о таблице
    const TableInfo* getTable(const std::string& tableName) const;
    
    // Проверить существование таблицы
    bool tableExists(const std::string& tableName) const;
    
    // Получить все имена таблиц
    std::vector<std::string> getTableNames() const;

private:
    std::unordered_map<std::string, TableInfo> tables_;
};

// Семантический анализатор
class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(SymbolTable& symbolTable);
    
    // Выполнить семантический анализ команды
    void analyze(const SQLCommand& cmd);
    
private:
    SymbolTable& symbolTable_;
    
    // Анализ различных команд
    void analyzeSelect(const SelectStmt& stmt);
    void analyzeInsert(const InsertStmt& stmt);
    void analyzeUpdate(const UpdateStmt& stmt);
    void analyzeDelete(const DeleteStmt& stmt);
    void analyzeCreateTable(const CreateTableStmt& stmt);
    void analyzeDropTable(const DropTableStmt& stmt);
    
    // Проверка условия WHERE
    void analyzeCondition(const Condition& cond, const TableInfo& table);
    
    // Проверка соответствия типа значения типу поля
    void checkTypeCompatibility(const std::string& fieldType, 
                                const std::string& value, 
                                bool isStringLiteral);
    
    // Получить тип из строкового представления
    static std::string normalizeType(const std::string& type);
};

} // namespace SQL

#endif // SQL_SEMANTIC_H
