#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include <memory>
#include <string>

namespace sql {

// Исключение для синтаксических ошибок
class ParseException : public std::runtime_error {
public:
    size_t line;
    size_t column;
    
    ParseException(const std::string& msg, size_t l, size_t c)
        : std::runtime_error(msg + " at line " + std::to_string(l) + ", column " + std::to_string(c)),
          line(l), column(c) {}
};

// Парсер методом рекурсивного спуска
class Parser {
public:
    explicit Parser(Lexer& lexer);
    
    // Разобрать SQL-предложение и вернуть корневой узел AST
    std::unique_ptr<SQLStatement> parse();
    
private:
    Lexer& lexer;
    
    // Вспомогательные методы для работы с токенами
    Token currentToken();
    Token consume(TokenType expected, const std::string& errorMsg);
    bool match(TokenType type);
    
    // Проверка длины имени поля
    void validateIdentifierLength(const std::string& name, size_t line, size_t col);
    
    // ========================================================================
    // Основные правила грамматики
    // ========================================================================
    
    // <SQL-предложение> ::= <SELECT-предложение> | <INSERT-предложение> | 
    //                       <UPDATE-предложение> | <DELETE-предложение> |
    //                       <CREATE-предложение> | <DROP-предложение>
    std::unique_ptr<SQLStatement> parseSQLStatement();
    
    // <SELECT-предложение> ::= SELECT <список полей> FROM <имя таблицы> <WHERE-клауза>
    std::unique_ptr<SelectStatement> parseSelectStatement();
    
    // <список полей> ::= <имя поля> { , <имя поля> } | *
    void parseFieldList(SelectStatement& stmt);
    
    // <INSERT-предложение> ::= INSERT INTO <имя таблицы> ( <значение поля> { , <значение поля> } )
    std::unique_ptr<InsertStatement> parseInsertStatement();
    
    // <UPDATE-предложение> ::= UPDATE <имя таблицы> SET <имя поля> = <выражение> <WHERE-клауза>
    std::unique_ptr<UpdateStatement> parseUpdateStatement();
    
    // <DELETE-предложение> ::= DELETE FROM <имя таблицы> <WHERE-клауза>
    std::unique_ptr<DeleteStatement> parseDeleteStatement();
    
    // <CREATE-предложение> ::= CREATE TABLE <имя таблицы> ( <список описаний полей> )
    std::unique_ptr<CreateTableStatement> parseCreateStatement();
    
    // <DROP-предложение> ::= DROP TABLE <имя таблицы>
    std::unique_ptr<DropTableStatement> parseDropStatement();
    
    // <WHERE-клауза> ::= WHERE <условие> | WHERE ALL | (отсутствует)
    std::unique_ptr<WhereClause> parseWhereClause();
    
    // <условие> ::= <имя поля типа TEXT> [ NOT ] LIKE <строка-образец>
    //             | <выражение> [ NOT ] IN ( <список констант> )
    //             | <логическое выражение>
    std::unique_ptr<WhereClause> parseCondition();
    
    // ========================================================================
    // Выражения
    // ========================================================================
    
    // <выражение> ::= <Long-выражение> | <Text-выражение>
    std::unique_ptr<Expression> parseExpression();
    
    // <Long-выражение> ::= <Long-слагаемое> { <+|-> <Long-слагаемое> }
    std::unique_ptr<LongExpression> parseLongExpression();
    
    // <Long-слагаемое> ::= <Long-множитель> { <*|/|%> <Long-множитель> }
    std::unique_ptr<LongExpression> parseLongTerm();
    
    // <Long-множитель> ::= <Long-величина> | ( <Long-выражение> )
    std::unique_ptr<LongExpression> parseLongFactor();
    
    // <Long-величина> ::= <имя поля типа LONG> | <длинное целое>
    std::unique_ptr<LongExpression> parseLongPrimary();
    
    // <Text-выражение> ::= <имя поля типа TEXT> | <строка>
    std::unique_ptr<TextExpression> parseTextExpression();
    
    // ========================================================================
    // Логические выражения и отношения
    // ========================================================================
    
    // <логическое выражение> ::= <логическое слагаемое> { OR <логическое слагаемое> }
    std::unique_ptr<LogicalExpression> parseLogicalExpression();
    
    // <логическое слагаемое> ::= <логический множитель> { AND <логический множитель> }
    std::unique_ptr<LogicalExpression> parseLogicalTerm();
    
    // <логический множитель> ::= NOT <логический множитель>
    //                          | ( <логическое выражение> )
    //                          | <отношение>
    std::unique_ptr<LogicalExpression> parseLogicalFactor();
    
    // <отношение> ::= <Text-отношение> | <Long-отношение>
    std::unique_ptr<LogicalExpression> parseRelation();
    
    // <Text-отношение> ::= <Text-выражение> <операция сравнения> <Text-выражение>
    // <Long-отношение> ::= <Long-выражение> <операция сравнения> <Long-выражение>
    std::unique_ptr<LogicalExpression> parseComparison();
    
    // <операция сравнения> ::= = | > | < | >= | <= | !=
    ComparisonOp::OpType parseComparisonOp();
    
    // ========================================================================
    // Вспомогательные правила
    // ========================================================================
    
    // <список описаний полей> ::= <описание поля> { , <описание поля> }
    void parseFieldDefinitions(CreateTableStatement& stmt);
    
    // <описание поля> ::= <имя поля> <тип поля>
    CreateTableStatement::FieldDef parseFieldDefinition();
    
    // <тип поля> ::= TEXT ( <целое без знака> ) | LONG
    CreateTableStatement::FieldDef parseFieldType(const std::string& fieldName);
    
    // <список констант> ::= <строка> { , <строка> } | <длинное целое> { , <длинное целое> }
    void parseConstantList(WhereIn& whereIn);
    
    // <имя> ::= идентификатор
    std::string parseIdentifier();
    
    // <строка> ::= строковый литерал
    std::string parseStringLiteral();
    
    // <длинное целое> ::= целый литерал
    long parseIntegerLiteral();
};

} // namespace sql

#endif // PARSER_H
