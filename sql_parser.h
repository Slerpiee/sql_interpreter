#ifndef SQL_PARSER_H
#define SQL_PARSER_H

#include "sql_tokens.h"
#include "sql_lexer.h"
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <stdexcept>

namespace SQL {

// Исключение для синтаксических ошибок
class ParserException : public std::runtime_error {
public:
    ParserException(const std::string& msg, const Token& token)
        : std::runtime_error(msg), token(token) {}
    
    const Token& getToken() const { return token; }
    
private:
    Token token;
};

// AST узлы для различных SQL команд

// Базовый класс для всех AST узлов
struct ASTNode {
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
};

// SELECT команда
struct SelectColumn {
    std::string name;      // имя колонки или *
    std::string tableAlias; // псевдоним таблицы (опционально)
    std::string alias;     // псевдоним колонки (AS ...)
};

struct Condition {
    std::string leftField;
    std::string op;        // =, !=, <, >, <=, >=
    std::string rightValue; // значение или поле
    bool isField;          // true если rightValue - это поле, false если литерал
    
    // Логические операторы для соединения условий
    std::string logicalOp; // AND, OR (пусто для первого условия)
};

struct SelectStmt : public ASTNode {
    std::vector<SelectColumn> columns;
    std::string tableName;
    std::vector<Condition> conditions;
    
    std::string toString() const override;
};

// INSERT команда
struct InsertValue {
    std::string value;
    bool isString;       // true если строковый литерал
    bool isNull;         // true если NULL
};

struct InsertStmt : public ASTNode {
    std::string tableName;
    std::vector<std::string> columns;  // имена колонок (опционально)
    std::vector<InsertValue> values;
    
    std::string toString() const override;
};

// UPDATE команда
struct UpdateAssignment {
    std::string column;
    std::string value;
    bool isString;       // true если строковый литерал
};

struct UpdateStmt : public ASTNode {
    std::string tableName;
    std::vector<UpdateAssignment> assignments;
    std::vector<Condition> conditions;
    
    std::string toString() const override;
};

// DELETE команда
struct DeleteStmt : public ASTNode {
    std::string tableName;
    std::vector<Condition> conditions;
    
    std::string toString() const override;
};

// CREATE TABLE команда
struct ColumnDef {
    std::string name;
    std::string type;    // LONG, TEXT(n)
    long size;           // размер для TEXT(n)
};

struct CreateTableStmt : public ASTNode {
    std::string tableName;
    std::vector<ColumnDef> columns;
    
    std::string toString() const override;
};

// DROP TABLE команда
struct DropTableStmt : public ASTNode {
    std::string tableName;
    
    std::string toString() const override;
};

// Тип команды
using SQLCommand = std::variant<
    std::unique_ptr<SelectStmt>,
    std::unique_ptr<InsertStmt>,
    std::unique_ptr<UpdateStmt>,
    std::unique_ptr<DeleteStmt>,
    std::unique_ptr<CreateTableStmt>,
    std::unique_ptr<DropTableStmt>
>;

// Парсер - преобразует токены в AST
class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    
    // Распарсить и вернуть команду
    SQLCommand parse();
    
private:
    std::vector<Token> tokens;
    size_t currentPos;
    
    // Получить текущий токен
    const Token& currentToken() const;
    
    // Получить предыдущий токен
    const Token& previousToken() const;
    
    // Проверить, достигнут ли конец
    bool isEnd() const { return currentPos >= tokens.size(); }
    
    // Перейти к следующему токену
    void advance();
    
    // Проверить тип текущего токена
    bool check(TokenType type) const;
    
    // Ожидать токен определенного типа
    Token expect(TokenType type, const std::string& errorMsg);
    
    // Методы парсинга для различных команд
    SQLCommand parseSelect();
    SQLCommand parseInsert();
    SQLCommand parseUpdate();
    SQLCommand parseDelete();
    SQLCommand parseCreateTable();
    SQLCommand parseDropTable();
    
    // Вспомогательные методы
    std::vector<SelectColumn> parseColumnList();
    std::vector<Condition> parseWhereClause();
    std::vector<UpdateAssignment> parseSetClause();
    std::vector<std::string> parseIdentifierList();
    std::vector<InsertValue> parseValuesList();
    std::vector<ColumnDef> parseColumnDefs();
    
    Condition parseCondition();
    InsertValue parseValue();
};

// Функция для удобного парсинга строки
SQLCommand parseSQL(const std::string& sql);

} // namespace SQL

#endif // SQL_PARSER_H
