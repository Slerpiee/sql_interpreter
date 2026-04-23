#ifndef DATABASE_H
#define DATABASE_H

#include "sql_types.h"
#include "sql_parser.h"
#include "_Table.h"
#include <string>
#include <map>
#include <memory>

// Класс для управления таблицей базы данных
class DatabaseTable {
public:
    DatabaseTable(const std::string& tableName);
    ~DatabaseTable();
    
    // Создание таблицы с заданной схемой
    bool create(const std::vector<FieldDefinition>& fields);
    
    // Открытие существующей таблицы
    bool open();
    
    // Закрытие таблицы
    void close();
    
    // Удаление таблицы
    bool drop();
    
    // Вставка новой записи
    bool insert(const std::map<std::string, SQLValue>& values);
    
    // Выборка записей
    SQLResult select(const std::vector<std::string>& columns, 
                     const std::vector<WhereCondition>& conditions);
    
    // Обновление записей
    int update(const std::map<std::string, SQLValue>& values,
               const std::vector<WhereCondition>& conditions);
    
    // Удаление записей
    int deleteRecords(const std::vector<WhereCondition>& conditions);
    
    // Получить схему таблицы
    std::vector<FieldDefinition> getSchema() const;
    
    // Проверка существования таблицы
    bool exists() const;
    
private:
    std::string tableName;
    THandle handle;
    std::vector<FieldDefinition> schema;
    bool isOpen;
    
    // Проверка условия WHERE для текущей записи
    bool checkCondition(THandle h, const WhereCondition& cond);
    
    // Сравнение значений
    bool compareValues(const SQLValue& left, const std::string& op, const SQLValue& right);
};

// Менеджер баз данных (управляет коллекцией таблиц)
class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();
    
    // Создать таблицу
    SQLResult createTable(const std::string& tableName, 
                          const std::vector<FieldDefinition>& fields);
    
    // Удалить таблицу
    SQLResult dropTable(const std::string& tableName);
    
    // Выполнить SELECT запрос
    SQLResult executeSelect(std::shared_ptr<SelectStatement> stmt);
    
    // Выполнить INSERT запрос
    SQLResult executeInsert(std::shared_ptr<InsertStatement> stmt);
    
    // Выполнить UPDATE запрос
    SQLResult executeUpdate(std::shared_ptr<UpdateStatement> stmt);
    
    // Выполнить DELETE запрос
    SQLResult executeDelete(std::shared_ptr<DeleteStatement> stmt);
    
    // Выполнить произвольный SQL-запрос
    SQLResult execute(const std::string& sql);
    
    // Получить последнюю ошибку
    std::string getLastError() const { return lastError; }
    
private:
    std::map<std::string, std::unique_ptr<DatabaseTable>> tables;
    std::string lastError;
    SQLParser parser;
    
    DatabaseTable* getOrCreateTable(const std::string& tableName);
};

#endif // DATABASE_H
