#ifndef SQL_PARSER_H
#define SQL_PARSER_H

#include "sql_types.h"
#include <string>
#include <vector>
#include <memory>

// Типы SQL-команд
enum class SQLCommandType {
    SELECT,
    INSERT,
    UPDATE,
    DELETE,
    CREATE_TABLE,
    DROP_TABLE,
    UNKNOWN
};

// Структура условия WHERE
struct WhereCondition {
    std::string column;
    std::string op;  // =, <>, <, >, <=, >=, LIKE
    SQLValue value;
    
    WhereCondition() {}
    WhereCondition(const std::string& c, const std::string& o, const SQLValue& v)
        : column(c), op(o), value(v) {}
};

// Базовый класс для SQL-запросов
class SQLStatement {
public:
    virtual ~SQLStatement() = default;
    virtual SQLCommandType getType() const = 0;
    virtual std::string toString() const = 0;
};

// SELECT запрос
class SelectStatement : public SQLStatement {
public:
    std::vector<std::string> columns;  // "*" или список колонок
    std::string table;
    std::vector<WhereCondition> whereConditions;
    bool hasWhere;
    
    SelectStatement() : hasWhere(false) {}
    
    SQLCommandType getType() const override { return SQLCommandType::SELECT; }
    
    std::string toString() const override {
        std::string result = "SELECT ";
        for (size_t i = 0; i < columns.size(); i++) {
            if (i > 0) result += ", ";
            result += columns[i];
        }
        result += " FROM " + table;
        if (hasWhere) {
            result += " WHERE ";
            for (size_t i = 0; i < whereConditions.size(); i++) {
                if (i > 0) result += " AND ";
                result += whereConditions[i].column + whereConditions[i].op + "...";
            }
        }
        return result;
    }
};

// INSERT запрос
class InsertStatement : public SQLStatement {
public:
    std::string table;
    std::vector<std::string> columns;
    std::vector<SQLValue> values;
    
    SQLCommandType getType() const override { return SQLCommandType::INSERT; }
    
    std::string toString() const override {
        std::string result = "INSERT INTO " + table + " (";
        for (size_t i = 0; i < columns.size(); i++) {
            if (i > 0) result += ", ";
            result += columns[i];
        }
        result += ") VALUES (...)";
        return result;
    }
};

// UPDATE запрос
class UpdateStatement : public SQLStatement {
public:
    std::string table;
    std::map<std::string, SQLValue> setValues;
    std::vector<WhereCondition> whereConditions;
    bool hasWhere;
    
    UpdateStatement() : hasWhere(false) {}
    
    SQLCommandType getType() const override { return SQLCommandType::UPDATE; }
    
    std::string toString() const override {
        std::string result = "UPDATE " + table + " SET ...";
        if (hasWhere) {
            result += " WHERE ...";
        }
        return result;
    }
};

// DELETE запрос
class DeleteStatement : public SQLStatement {
public:
    std::string table;
    std::vector<WhereCondition> whereConditions;
    bool hasWhere;
    
    DeleteStatement() : hasWhere(false) {}
    
    SQLCommandType getType() const override { return SQLCommandType::DELETE; }
    
    std::string toString() const override {
        std::string result = "DELETE FROM " + table;
        if (hasWhere) {
            result += " WHERE ...";
        }
        return result;
    }
};

// CREATE TABLE запрос
class CreateTableStatement : public SQLStatement {
public:
    std::string tableName;
    std::vector<FieldDefinition> fields;
    
    SQLCommandType getType() const override { return SQLCommandType::CREATE_TABLE; }
    
    std::string toString() const override {
        std::string result = "CREATE TABLE " + tableName + " (";
        for (size_t i = 0; i < fields.size(); i++) {
            if (i > 0) result += ", ";
            result += fields[i].name + " ";
            switch (fields[i].type) {
                case SQLType::INTEGER: result += "INTEGER"; break;
                case SQLType::VARCHAR: result += "VARCHAR(" + std::to_string(fields[i].length) + ")"; break;
                default: result += "UNKNOWN";
            }
        }
        result += ")";
        return result;
    }
};

// DROP TABLE запрос
class DropTableStatement : public SQLStatement {
public:
    std::string tableName;
    
    SQLCommandType getType() const override { return SQLCommandType::DROP_TABLE; }
    
    std::string toString() const override {
        return "DROP TABLE " + tableName;
    }
};

// Парсер SQL
class SQLParser {
public:
    SQLParser();
    ~SQLParser();
    
    // Разбор SQL-запроса
    std::shared_ptr<SQLStatement> parse(const std::string& sql);
    
    // Получить ошибку последней операции
    std::string getLastError() const { return lastError; }
    
private:
    std::string lastError;
    size_t pos;
    std::string input;
    
    // Вспомогательные методы
    void skipWhitespace();
    std::string readKeyword();
    std::string readIdentifier();
    std::string readQuotedString();
    SQLValue readValue();
    std::string readOperator();
    
    // Методы разбора конкретных команд
    std::shared_ptr<SelectStatement> parseSelect();
    std::shared_ptr<InsertStatement> parseInsert();
    std::shared_ptr<UpdateStatement> parseUpdate();
    std::shared_ptr<DeleteStatement> parseDelete();
    std::shared_ptr<CreateTableStatement> parseCreateTable();
    std::shared_ptr<DropTableStatement> parseDropTable();
    
    // Разбор условий WHERE
    std::vector<WhereCondition> parseWhereClause();
    WhereCondition parseCondition();
};

#endif // SQL_PARSER_H
