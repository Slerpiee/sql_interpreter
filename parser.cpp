#include "parser.h"
#include <cstdlib>
#include <sstream>

namespace sql {

Parser::Parser(Lexer& lexer) : lexer(lexer) {}

Token Parser::currentToken() {
    return lexer.peek();
}

Token Parser::consume(TokenType expected, const std::string& errorMsg) {
    Token token = lexer.nextToken();
    if (token.type != expected) {
        throw ParseException(errorMsg + ", got '" + token.value + "'", 
                            token.line, token.column);
    }
    return token;
}

bool Parser::match(TokenType type) {
    Token token = currentToken();
    if (token.type == type) {
        lexer.nextToken();
        return true;
    }
    return false;
}

void Parser::validateIdentifierLength(const std::string& name, size_t line, size_t col) {
    if (name.length() > MaxFieldNameLen) {
        std::ostringstream oss;
        oss << "Identifier '" << name << "' exceeds maximum length of " 
            << MaxFieldNameLen << " characters";
        throw ParseException(oss.str(), line, col);
    }
}

// ============================================================================
// <SQL-предложение> ::= <SELECT-предложение> | <INSERT-предложение> | 
//                       <UPDATE-предложение> | <DELETE-предложение> |
//                       <CREATE-предложение> | <DROP-предложение>
// ============================================================================
std::unique_ptr<SQLStatement> Parser::parseSQLStatement() {
    Token token = currentToken();
    
    switch (token.type) {
        case TokenType::KEYWORD_SELECT:
            return parseSelectStatement();
        case TokenType::KEYWORD_INSERT:
            return parseInsertStatement();
        case TokenType::KEYWORD_UPDATE:
            return parseUpdateStatement();
        case TokenType::KEYWORD_DELETE:
            return parseDeleteStatement();
        case TokenType::KEYWORD_CREATE:
            return parseCreateStatement();
        case TokenType::KEYWORD_DROP:
            return parseDropStatement();
        default:
            throw ParseException("Expected SQL statement keyword", 
                                token.line, token.column);
    }
}

// ============================================================================
// Парсинг основного запроса
// ============================================================================
std::unique_ptr<SQLStatement> Parser::parse() {
    auto stmt = parseSQLStatement();
    
    // Опционально пропускаем точку с запятой
    match(TokenType::SEMICOLON);
    
    // Проверяем, что достигли конца ввода
    Token eof = currentToken();
    if (eof.type != TokenType::EOF_TOKEN) {
        throw ParseException("Unexpected tokens after statement", 
                            eof.line, eof.column);
    }
    
    return stmt;
}

// ============================================================================
// <SELECT-предложение> ::= SELECT <список полей> FROM <имя таблицы> <WHERE-клауза>
// ============================================================================
std::unique_ptr<SelectStatement> Parser::parseSelectStatement() {
    consume(TokenType::KEYWORD_SELECT, "Expected SELECT");
    
    auto stmt = std::make_unique<SelectStatement>();
    
    // Проверяем на SELECT *
    if (match(TokenType::STAR)) {
        stmt->selectAll = true;
    } else {
        parseFieldList(*stmt);
    }
    
    consume(TokenType::KEYWORD_FROM, "Expected FROM");
    
    stmt->tableName = parseIdentifier();
    validateIdentifierLength(stmt->tableName, currentToken().line, currentToken().column);
    
    // WHERE клауза опциональна
    if (match(TokenType::KEYWORD_WHERE)) {
        stmt->whereClause = parseWhereClause();
    }
    
    return stmt;
}

// <список полей> ::= <имя поля> { , <имя поля> } | *
void Parser::parseFieldList(SelectStatement& stmt) {
    std::string fieldName = parseIdentifier();
    validateIdentifierLength(fieldName, currentToken().line, currentToken().column);
    stmt.addField(fieldName);
    
    while (match(TokenType::COMMA)) {
        fieldName = parseIdentifier();
        validateIdentifierLength(fieldName, currentToken().line, currentToken().column);
        stmt.addField(fieldName);
    }
}

// ============================================================================
// <INSERT-предложение> ::= INSERT INTO <имя таблицы> ( <значение поля> { , <значение поля> } )
// ============================================================================
std::unique_ptr<InsertStatement> Parser::parseInsertStatement() {
    consume(TokenType::KEYWORD_INSERT, "Expected INSERT");
    consume(TokenType::KEYWORD_INTO, "Expected INTO");
    
    auto stmt = std::make_unique<InsertStatement>(parseIdentifier());
    validateIdentifierLength(stmt->tableName, currentToken().line, currentToken().column);
    
    consume(TokenType::LPAREN, "Expected '(' after table name");
    
    // Первое значение
    stmt->addValue(parseExpression());
    
    // Остальные значения через запятую
    while (match(TokenType::COMMA)) {
        stmt->addValue(parseExpression());
    }
    
    consume(TokenType::RPAREN, "Expected ')' after values");
    
    return stmt;
}

// ============================================================================
// <UPDATE-предложение> ::= UPDATE <имя таблицы> SET <имя поля> = <выражение> <WHERE-клауза>
// ============================================================================
std::unique_ptr<UpdateStatement> Parser::parseUpdateStatement() {
    consume(TokenType::KEYWORD_UPDATE, "Expected UPDATE");
    
    auto stmt = std::make_unique<UpdateStatement>();
    stmt->tableName = parseIdentifier();
    validateIdentifierLength(stmt->tableName, currentToken().line, currentToken().column);
    
    consume(TokenType::KEYWORD_SET, "Expected SET");
    
    stmt->fieldName = parseIdentifier();
    validateIdentifierLength(stmt->fieldName, currentToken().line, currentToken().column);
    
    consume(TokenType::EQUAL, "Expected '=' after field name");
    
    stmt->value = parseExpression();
    
    // WHERE клауза опциональна
    if (match(TokenType::KEYWORD_WHERE)) {
        stmt->whereClause = parseWhereClause();
    }
    
    return stmt;
}

// ============================================================================
// <DELETE-предложение> ::= DELETE FROM <имя таблицы> <WHERE-клауза>
// ============================================================================
std::unique_ptr<DeleteStatement> Parser::parseDeleteStatement() {
    consume(TokenType::KEYWORD_DELETE, "Expected DELETE");
    consume(TokenType::KEYWORD_FROM, "Expected FROM");
    
    auto stmt = std::make_unique<DeleteStatement>(parseIdentifier());
    validateIdentifierLength(stmt->tableName, currentToken().line, currentToken().column);
    
    // WHERE клауза опциональна
    if (match(TokenType::KEYWORD_WHERE)) {
        stmt->whereClause = parseWhereClause();
    }
    
    return stmt;
}

// ============================================================================
// <CREATE-предложение> ::= CREATE TABLE <имя таблицы> ( <список описаний полей> )
// ============================================================================
std::unique_ptr<CreateTableStatement> Parser::parseCreateStatement() {
    consume(TokenType::KEYWORD_CREATE, "Expected CREATE");
    consume(TokenType::KEYWORD_TABLE, "Expected TABLE");
    
    auto stmt = std::make_unique<CreateTableStatement>(parseIdentifier());
    validateIdentifierLength(stmt->tableName, currentToken().line, currentToken().column);
    
    consume(TokenType::LPAREN, "Expected '(' after table name");
    
    parseFieldDefinitions(*stmt);
    
    consume(TokenType::RPAREN, "Expected ')' after field definitions");
    
    return stmt;
}

// <список описаний полей> ::= <описание поля> { , <описание поля> }
void Parser::parseFieldDefinitions(CreateTableStatement& stmt) {
    stmt.addField(parseFieldDefinition());
    
    while (match(TokenType::COMMA)) {
        stmt.addField(parseFieldDefinition());
    }
}

// <описание поля> ::= <имя поля> <тип поля>
CreateTableStatement::FieldDef Parser::parseFieldDefinition() {
    std::string fieldName = parseIdentifier();
    validateIdentifierLength(fieldName, currentToken().line, currentToken().column);
    return parseFieldType(fieldName);
}

// <тип поля> ::= TEXT ( <целое без знака> ) | LONG
CreateTableStatement::FieldDef Parser::parseFieldType(const std::string& fieldName) {
    CreateTableStatement::FieldDef def;
    def.name = fieldName;
    
    Token token = currentToken();
    
    if (match(TokenType::KEYWORD_TEXT)) {
        def.isText = true;
        consume(TokenType::LPAREN, "Expected '(' after TEXT");
        
        Token numToken = consume(TokenType::INTEGER_LITERAL, 
                                 "Expected unsigned integer for TEXT length");
        
        // Проверяем, что число неотрицательное
        long len = std::strtol(numToken.value.c_str(), nullptr, 10);
        if (len < 0) {
            throw ParseException("TEXT length must be non-negative", 
                                numToken.line, numToken.column);
        }
        def.textLen = static_cast<unsigned>(len);
        
        consume(TokenType::RPAREN, "Expected ')' after TEXT length");
    } else if (match(TokenType::KEYWORD_LONG)) {
        def.isText = false;
        def.textLen = 0;
    } else {
        throw ParseException("Expected TEXT or LONG type", 
                            token.line, token.column);
    }
    
    return def;
}

// ============================================================================
// <DROP-предложение> ::= DROP TABLE <имя таблицы>
// ============================================================================
std::unique_ptr<DropTableStatement> Parser::parseDropStatement() {
    consume(TokenType::KEYWORD_DROP, "Expected DROP");
    consume(TokenType::KEYWORD_TABLE, "Expected TABLE");
    
    auto stmt = std::make_unique<DropTableStatement>(parseIdentifier());
    validateIdentifierLength(stmt->tableName, currentToken().line, currentToken().column);
    
    return stmt;
}

// ============================================================================
// <WHERE-клауза> ::= WHERE <условие> | WHERE ALL | (отсутствует)
// ============================================================================
std::unique_ptr<WhereClause> Parser::parseWhereClause() {
    Token token = currentToken();
    
    // WHERE ALL - все строки
    if (match(TokenType::KEYWORD_ALL)) {
        return std::make_unique<WhereAll>();
    }
    
    // Иначе парсим условие
    return parseCondition();
}

// <условие> ::= <имя поля типа TEXT> [ NOT ] LIKE <строка-образец>
//             | <выражение> [ NOT ] IN ( <список констант> )
//             | <логическое выражение>
std::unique_ptr<WhereClause> Parser::parseCondition() {
    Token token = currentToken();
    
    // Пробуем распарсить как логическое выражение (включая отношения)
    auto logicalExpr = parseLogicalExpression();
    
    // Проверяем, не было ли это на самом деле LIKE или IN
    // Это определяется внутри parseRelation -> parseComparison
    
    return std::make_unique<WhereLogical>(std::move(logicalExpr));
}

// ============================================================================
// <выражение> ::= <Long-выражение> | <Text-выражение>
// ============================================================================
std::unique_ptr<Expression> Parser::parseExpression() {
    Token token = currentToken();
    
    // Если строковый литерал - это текстовое выражение
    if (token.type == TokenType::STRING_LITERAL) {
        return parseTextExpression();
    }
    
    // Иначе пробуем как Long-выражение
    // (идентификатор может быть и тем, и другим, но начинаем с Long)
    return parseLongExpression();
}

// ============================================================================
// <Long-выражение> ::= <Long-слагаемое> { <+|-> <Long-слагаемое> }
// ============================================================================
std::unique_ptr<LongExpression> Parser::parseLongExpression() {
    auto left = parseLongTerm();
    
    while (true) {
        Token opToken = currentToken();
        
        LongBinaryOp::OpType op;
        if (match(TokenType::PLUS)) {
            op = LongBinaryOp::ADD;
        } else if (match(TokenType::MINUS)) {
            op = LongBinaryOp::SUB;
        } else {
            break;
        }
        
        auto right = parseLongTerm();
        left = std::make_unique<LongBinaryOp>(op, std::move(left), std::move(right));
    }
    
    return left;
}

// <Long-слагаемое> ::= <Long-множитель> { <*|/|%> <Long-множитель> }
std::unique_ptr<LongExpression> Parser::parseLongTerm() {
    auto left = parseLongFactor();
    
    while (true) {
        Token opToken = currentToken();
        
        LongBinaryOp::OpType op;
        if (match(TokenType::STAR)) {
            op = LongBinaryOp::MUL;
        } else if (match(TokenType::SLASH)) {
            op = LongBinaryOp::DIV;
        } else if (match(TokenType::PERCENT)) {
            op = LongBinaryOp::MOD;
        } else {
            break;
        }
        
        auto right = parseLongFactor();
        left = std::make_unique<LongBinaryOp>(op, std::move(left), std::move(right));
    }
    
    return left;
}

// <Long-множитель> ::= <Long-величина> | ( <Long-выражение> )
std::unique_ptr<LongExpression> Parser::parseLongFactor() {
    Token token = currentToken();
    
    if (match(TokenType::LPAREN)) {
        auto expr = parseLongExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    return parseLongPrimary();
}

// <Long-величина> ::= <имя поля типа LONG> | <длинное целое>
std::unique_ptr<LongExpression> Parser::parseLongPrimary() {
    Token token = currentToken();
    
    if (token.type == TokenType::INTEGER_LITERAL) {
        lexer.nextToken();
        long value = std::strtol(token.value.c_str(), nullptr, 10);
        return std::make_unique<IntegerLiteral>(value);
    }
    
    if (token.type == TokenType::IDENTIFIER) {
        lexer.nextToken();
        validateIdentifierLength(token.value, token.line, token.column);
        return std::make_unique<Identifier>(token.value);
    }
    
    throw ParseException("Expected integer or identifier", 
                        token.line, token.column);
}

// <Text-выражение> ::= <имя поля типа TEXT> | <строка>
std::unique_ptr<TextExpression> Parser::parseTextExpression() {
    Token token = currentToken();
    
    if (token.type == TokenType::STRING_LITERAL) {
        lexer.nextToken();
        return std::make_unique<StringLiteral>(token.value);
    }
    
    // Для текстовых выражений идентификаторы не поддерживаются напрямую
    // так как Identifier теперь только LongExpression
    throw ParseException("Expected string literal for text expression", 
                        token.line, token.column);
}

// ============================================================================
// Логические выражения
// ============================================================================

// <логическое выражение> ::= <логическое слагаемое> { OR <логическое слагаемое> }
std::unique_ptr<LogicalExpression> Parser::parseLogicalExpression() {
    auto left = parseLogicalTerm();
    
    while (match(TokenType::KEYWORD_OR)) {
        auto right = parseLogicalTerm();
        left = std::make_unique<LogicalOr>(std::move(left), std::move(right));
    }
    
    return left;
}

// <логическое слагаемое> ::= <логический множитель> { AND <логический множитель> }
std::unique_ptr<LogicalExpression> Parser::parseLogicalTerm() {
    auto left = parseLogicalFactor();
    
    while (match(TokenType::KEYWORD_AND)) {
        auto right = parseLogicalFactor();
        left = std::make_unique<LogicalAnd>(std::move(left), std::move(right));
    }
    
    return left;
}

// <логический множитель> ::= NOT <логический множитель>
//                          | ( <логическое выражение> )
//                          | <отношение>
std::unique_ptr<LogicalExpression> Parser::parseLogicalFactor() {
    Token token = currentToken();
    
    if (match(TokenType::KEYWORD_NOT)) {
        auto operand = parseLogicalFactor();
        return std::make_unique<LogicalNot>(std::move(operand));
    }
    
    if (match(TokenType::LPAREN)) {
        // Вложенное логическое выражение в скобках
        auto expr = parseLogicalExpression();
        consume(TokenType::RPAREN, "Expected ')' after logical expression");
        return expr;
    }
    
    // <отношение>
    return parseRelation();
}

// ============================================================================
// Отношения (сравнения)
// ============================================================================

// <отношение> ::= <Text-отношение> | <Long-отношение>
std::unique_ptr<LogicalExpression> Parser::parseRelation() {
    return parseComparison();
}

// <операция сравнения> ::= = | > | < | >= | <= | !=
ComparisonOp::OpType Parser::parseComparisonOp() {
    Token token = currentToken();
    
    if (match(TokenType::EQUAL)) {
        return ComparisonOp::EQ;
    }
    if (match(TokenType::NOT_EQUAL)) {
        return ComparisonOp::NE;
    }
    if (match(TokenType::LESS)) {
        return ComparisonOp::LT;
    }
    if (match(TokenType::LESS_EQUAL)) {
        return ComparisonOp::LE;
    }
    if (match(TokenType::GREATER)) {
        return ComparisonOp::GT;
    }
    if (match(TokenType::GREATER_EQUAL)) {
        return ComparisonOp::GE;
    }
    
    throw ParseException("Expected comparison operator", 
                        token.line, token.column);
}

// <Text-отношение> ::= <Text-выражение> <операция сравнения> <Text-выражение>
// <Long-отношение> ::= <Long-выражение> <операция сравнения> <Long-выражение>
std::unique_ptr<LogicalExpression> Parser::parseComparison() {
    Token token = currentToken();
    
    // Пробуем начать с текстового выражения (строка или идентификатор)
    std::unique_ptr<Expression> left;
    bool isTextComparison = false;
    
    if (token.type == TokenType::STRING_LITERAL) {
        left = parseTextExpression();
        isTextComparison = true;
    } else if (token.type == TokenType::IDENTIFIER) {
        // Идентификатор может быть и текстовым, и числовым полем
        lexer.nextToken();
        validateIdentifierLength(token.value, token.line, token.column);
        // Создаём как LongExpression, потом приводим к Expression через unique_ptr с базовым типом
        auto longExpr = std::make_unique<Identifier>(token.value);
        left = std::unique_ptr<Expression>(static_cast<Expression*>(longExpr.release()));
    } else if (token.type == TokenType::INTEGER_LITERAL || 
               token.type == TokenType::MINUS ||
               token.type == TokenType::LPAREN) {
        left = parseLongExpression();
    } else {
        throw ParseException("Expected expression in comparison", 
                            token.line, token.column);
    }
    
    // Операция сравнения
    ComparisonOp::OpType op = parseComparisonOp();
    
    // Правое выражение
    std::unique_ptr<Expression> right;
    token = currentToken();
    
    if (token.type == TokenType::STRING_LITERAL) {
        right = parseTextExpression();
        isTextComparison = true;
    } else if (token.type == TokenType::IDENTIFIER) {
        lexer.nextToken();
        validateIdentifierLength(token.value, token.line, token.column);
        auto longExpr = std::make_unique<Identifier>(token.value);
        right = std::unique_ptr<Expression>(static_cast<Expression*>(longExpr.release()));
    } else if (token.type == TokenType::INTEGER_LITERAL || 
               token.type == TokenType::MINUS ||
               token.type == TokenType::LPAREN) {
        right = parseLongExpression();
    } else {
        throw ParseException("Expected expression after comparison operator", 
                            token.line, token.column);
    }
    
    return std::unique_ptr<LogicalExpression>(
        static_cast<LogicalExpression*>(new ComparisonOp(op, std::move(left), std::move(right), isTextComparison)));
}

// ============================================================================
// Вспомогательные функции
// ============================================================================

std::string Parser::parseIdentifier() {
    Token token = consume(TokenType::IDENTIFIER, "Expected identifier");
    return token.value;
}

std::string Parser::parseStringLiteral() {
    Token token = consume(TokenType::STRING_LITERAL, "Expected string literal");
    return token.value;
}

long Parser::parseIntegerLiteral() {
    Token token = consume(TokenType::INTEGER_LITERAL, "Expected integer");
    return std::strtol(token.value.c_str(), nullptr, 10);
}

} // namespace sql
