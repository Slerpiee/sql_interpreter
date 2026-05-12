#ifndef SQL_EXECUTOR_H
#define SQL_EXECUTOR_H

#include "sql_semantic.h"
#include "TableWrapper.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace SQL {

// Результат выполнения запроса
struct QueryResult {
    bool success;
    std::string message;
    std::vector<std::string> columnNames;
    std::vector<std::vector<std::string>> rows;
    int affectedRows;
    
    QueryResult() : success(true), affectedRows(0) {}
};

// Исключение исполнителя
class ExecutorException : public std::runtime_error {
public:
    explicit ExecutorException(const std::string& msg) 
        : std::runtime_error(msg) {}
};

// Исполнитель SQL команд
class Executor {
public:
    Executor();
    ~Executor();
    
    // Выполнить SQL команду (строку)
    QueryResult execute(const std::string& sql);
    
    // Получить список всех таблиц
    std::vector<std::string> getTables() const;
    
    // Получить информацию о таблице
    struct TableSchema {
        std::string name;
        std::string type;  // LONG или TEXT
        long size;
    };
    std::vector<TableSchema> getTableSchema(const std::string& tableName) const;

private:
    SymbolTable symbolTable_;
    std::string currentDatabase_;  // директория для хранения таблиц
    
    // Методы выполнения различных команд
    QueryResult executeSelect(const SelectStmt& stmt);
    QueryResult executeInsert(const InsertStmt& stmt);
    QueryResult executeUpdate(const UpdateStmt& stmt);
    QueryResult executeDelete(const DeleteStmt& stmt);
    QueryResult executeCreateTable(const CreateTableStmt& stmt);
    QueryResult executeDropTable(const DropTableStmt& stmt);
    
    // Вспомогательные методы
    bool evaluateCondition(const Condition& cond, 
                          TableLib::Table& table,
                          const TableInfo& tableInfo);
    std::string getFieldValue(TableLib::Table& table, 
                             const std::string& fieldName,
                             const FieldInfo& fieldInfo);
    void setFieldValue(TableLib::Table& table,
                      const std::string& fieldName,
                      const std::string& value,
                      const FieldInfo& fieldInfo);
    
    // Преобразование типов
    static std::string normalizeType(const std::string& type);
    static bool isNumeric(const std::string& str);
};

// Удобная функция для выполнения SQL
QueryResult executeSQL(const std::string& sql);

} // namespace SQL

#endif // SQL_EXECUTOR_H
