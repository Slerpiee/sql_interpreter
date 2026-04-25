#include "sql_parser.h"
#include <sstream>

namespace SQL {

// Реализация toString для AST узлов

std::string SelectStmt::toString() const {
    std::ostringstream oss;
    oss << "SELECT ";
    
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) oss << ", ";
        if (!columns[i].tableAlias.empty()) {
            oss << columns[i].tableAlias << ".";
        }
        oss << columns[i].name;
        if (!columns[i].alias.empty()) {
            oss << " AS " << columns[i].alias;
        }
    }
    
    oss << " FROM " << tableName;
    
    if (!conditions.empty()) {
        oss << " WHERE ";
        for (size_t i = 0; i < conditions.size(); ++i) {
            if (i > 0) {
                oss << " " << conditions[i].logicalOp << " ";
            }
            oss << conditions[i].leftField << " " << conditions[i].op << " ";
            if (conditions[i].isField) {
                oss << conditions[i].rightValue;
            } else {
                oss << "'" << conditions[i].rightValue << "'";
            }
        }
    }
    
    return oss.str();
}

std::string InsertStmt::toString() const {
    std::ostringstream oss;
    oss << "INSERT INTO " << tableName;
    
    if (!columns.empty()) {
        oss << " (";
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << columns[i];
        }
        oss << ")";
    }
    
    oss << " VALUES (";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) oss << ", ";
        if (values[i].isNull) {
            oss << "NULL";
        } else if (values[i].isString) {
            oss << "'" << values[i].value << "'";
        } else {
            oss << values[i].value;
        }
    }
    oss << ")";
    
    return oss.str();
}

std::string UpdateStmt::toString() const {
    std::ostringstream oss;
    oss << "UPDATE " << tableName << " SET ";
    
    for (size_t i = 0; i < assignments.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << assignments[i].column << " = ";
        if (assignments[i].isString) {
            oss << "'" << assignments[i].value << "'";
        } else {
            oss << assignments[i].value;
        }
    }
    
    if (!conditions.empty()) {
        oss << " WHERE ";
        for (size_t i = 0; i < conditions.size(); ++i) {
            if (i > 0) {
                oss << " " << conditions[i].logicalOp << " ";
            }
            oss << conditions[i].leftField << " " << conditions[i].op << " ";
            if (conditions[i].isField) {
                oss << conditions[i].rightValue;
            } else {
                oss << "'" << conditions[i].rightValue << "'";
            }
        }
    }
    
    return oss.str();
}

std::string DeleteStmt::toString() const {
    std::ostringstream oss;
    oss << "DELETE FROM " << tableName;
    
    if (!conditions.empty()) {
        oss << " WHERE ";
        for (size_t i = 0; i < conditions.size(); ++i) {
            if (i > 0) {
                oss << " " << conditions[i].logicalOp << " ";
            }
            oss << conditions[i].leftField << " " << conditions[i].op << " ";
            if (conditions[i].isField) {
                oss << conditions[i].rightValue;
            } else {
                oss << "'" << conditions[i].rightValue << "'";
            }
        }
    }
    
    return oss.str();
}

std::string CreateTableStmt::toString() const {
    std::ostringstream oss;
    oss << "CREATE TABLE " << tableName << " (";
    
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << columns[i].name << " " << columns[i].type;
        if (columns[i].size > 0) {
            oss << "(" << columns[i].size << ")";
        }
    }
    
    oss << ")";
    return oss.str();
}

std::string DropTableStmt::toString() const {
    return "DROP TABLE " + tableName;
}

// Реализация парсера

Parser::Parser(const std::vector<Token>& tokens)
    : tokens(tokens), currentPos(0) {}

// Получить текущий токен
const Token& Parser::currentToken() const {
    if (currentPos >= tokens.size()) {
        static Token endToken(TokenType::END_OF_INPUT);
        return endToken;
    }
    return tokens[currentPos];
}

// Получить предыдущий токен
const Token& Parser::previousToken() const {
    static Token dummy(TokenType::UNKNOWN);
    if (currentPos == 0) return dummy;
    return tokens[currentPos - 1];
}

// Перейти к следующему токену
void Parser::advance() {
    if (!isEnd()) {
        currentPos++;
    }
}

// Проверить тип текущего токена
bool Parser::check(TokenType type) const {
    return currentToken().type == type;
}

// Ожидать токен определенного типа
Token Parser::expect(TokenType type, const std::string& errorMsg) {
    if (!check(type)) {
        throw ParserException(errorMsg, currentToken());
    }
    Token token = currentToken();
    advance();
    return token;
}

// Распарсить и вернуть команду
SQLCommand Parser::parse() {
    if (isEnd()) {
        throw ParserException("Empty input", currentToken());
    }
    
    switch (currentToken().type) {
        case TokenType::SELECT:
            return parseSelect();
        
        case TokenType::INSERT:
            return parseInsert();
        
        case TokenType::UPDATE:
            return parseUpdate();
        
        case TokenType::DELETE:
            return parseDelete();
        
        case TokenType::CREATE:
            return parseCreateTable();
        
        case TokenType::DROP:
            return parseDropTable();
        
        default:
            throw ParserException("Unexpected token: " + currentToken().value, currentToken());
    }
}

std::vector<SelectColumn> Parser::parseColumnList() {
    std::vector<SelectColumn> columns;
    
    do {
        SelectColumn col;
        
        // Опциональный псевдоним таблицы
        if (check(TokenType::IDENTIFIER)) {
            std::string first = currentToken().value;
            advance();
            
            if (check(TokenType::DOT)) {
                advance(); // пропускаем точку
                if (!check(TokenType::IDENTIFIER)) {
                    throw ParserException("Expected column name after '.'", currentToken());
                }
                col.tableAlias = first;
                col.name = currentToken().value;
                advance();
            } else {
                col.name = first;
            }
        } else if (check(TokenType::STAR)) {
            col.name = "*";
            advance();
        } else {
            throw ParserException("Expected column name or '*'", currentToken());
        }

        
        
        
        if (check(TokenType::AS)) {
            advance(); //пропускаем AS
            if (!check(TokenType::IDENTIFIER)) {
                throw ParserException("Expected alias name after AS", currentToken());
            }
            col.alias = currentToken().value;
            advance();
        } else if (check(TokenType::IDENTIFIER)) {
            std::string upperVal = currentToken().value;
            for (char& c : upperVal) c = std::toupper(static_cast<unsigned char>(c));
            
            // Если это FROM - значит предыдущий идентификатор был таблицей без alias
            if (upperVal == "FROM") {
                // ничего не делаем, это конец списка колонок
            } else if (upperVal != "WHERE" && upperVal != "ORDER" && 
                       upperVal != "GROUP" && upperVal != "HAVING" && upperVal != "LIMIT" &&
                       upperVal != "AND" && upperVal != "OR") {
                col.alias = currentToken().value;
                advance();
            }
        }
        
        columns.push_back(col);
        
        if (check(TokenType::COMMA)) {
            advance();
        } else {
            break;
        }
    } while (!isEnd());
    
    return columns;
}

Condition Parser::parseCondition() {
    Condition cond;
    
    expect(TokenType::IDENTIFIER, "Expected field name in condition");
    cond.leftField = currentToken().value;
    advance();
    
    // Оператор сравнения
    switch (currentToken().type) {
        case TokenType::EQUALS:
            cond.op = "=";
            break;
        case TokenType::NOT_EQUALS:
            cond.op = "!=";
            break;
        case TokenType::LESS:
            cond.op = "<";
            break;
        case TokenType::GREATER:
            cond.op = ">";
            break;
        case TokenType::LESS_EQ:
            cond.op = "<=";
            break;
        case TokenType::GREATER_EQ:
            cond.op = ">=";
            break;
        default:
            throw ParserException("Expected comparison operator", currentToken());
    }
    advance();
    
    // Значение или поле
    if (check(TokenType::IDENTIFIER)) {
        cond.rightValue = currentToken().value;
        cond.isField = true;
        advance();
    } else if (check(TokenType::STRING_LITERAL)) {
        cond.rightValue = currentToken().value;
        cond.isField = false;
        advance();
    } else if (check(TokenType::INTEGER_LITERAL)) {
        cond.rightValue = currentToken().value;
        cond.isField = false;
        advance();
    } else {
        throw ParserException("Expected value or field in condition", currentToken());
    }
    
    // Логический оператор
    if (check(TokenType::AND)) {
        cond.logicalOp = "AND";
    } else if (check(TokenType::OR)) {
        cond.logicalOp = "OR";
    }
    
    return cond;
}

std::vector<Condition> Parser::parseWhereClause() {
    std::vector<Condition> conditions;
    
    expect(TokenType::WHERE, "Expected WHERE keyword");
    
    do {
        Condition cond = parseCondition();
        
        // Если это не первое условие, добавляем логический оператор от предыдущего
        if (!conditions.empty() && !cond.logicalOp.empty()) {
            conditions.back().logicalOp = cond.logicalOp;
            cond.logicalOp.clear();
        }
        
        conditions.push_back(cond);
        
        // Продолжаем, если есть AND/OR
        if (check(TokenType::AND) || check(TokenType::OR)) {
            advance();
        } else {
            break;
        }
    } while (!isEnd());
    
    return conditions;
}

SQLCommand Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStmt>();
    
    advance(); // пропускаем SELECT
    
    // Список колонок
    stmt->columns = parseColumnList();
    
    // FROM
    expect(TokenType::FROM, "Expected FROM keyword");
    expect(TokenType::IDENTIFIER, "Expected table name after FROM");
    stmt->tableName = currentToken().value;
    advance();
    
    // Опциональное WHERE
    if (check(TokenType::WHERE)) {
        stmt->conditions = parseWhereClause();
    }
    
    return stmt;
}

InsertValue Parser::parseValue() {
    InsertValue val;
    val.isNull = false;
    val.isString = false;
    
    if (check(TokenType::STRING_LITERAL)) {
        val.value = currentToken().value;
        val.isString = true;
        advance();
    } else if (check(TokenType::INTEGER_LITERAL)) {
        val.value = currentToken().value;
        advance();
    } else if (check(TokenType::IDENTIFIER)) {
        std::string upperVal = currentToken().value;
        for (char& c : upperVal) c = std::toupper(static_cast<unsigned char>(c));
        if (upperVal == "NULL") {
            val.isNull = true;
            advance();
        } else {
            throw ParserException("Expected value", currentToken());
        }
    } else {
        throw ParserException("Expected value", currentToken());
    }
    
    return val;
}

std::vector<InsertValue> Parser::parseValuesList() {
    std::vector<InsertValue> values;
    
    expect(TokenType::LPAREN, "Expected '(' after VALUES");
    
    do {
        values.push_back(parseValue());
        
        if (check(TokenType::COMMA)) {
            advance();
        } else {
            break;
        }
    } while (!isEnd());
    
    expect(TokenType::RPAREN, "Expected ')' after values");
    
    return values;
}

SQLCommand Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStmt>();
    
    advance(); // пропускаем INSERT
    
    if (!check(TokenType::INTO)) {
        throw ParserException("Expected INTO after INSERT", currentToken());
    }
    advance(); // пропускаем INTO
    
    if (!check(TokenType::IDENTIFIER)) {
        throw ParserException("Expected table name after INTO", currentToken());
    }
    stmt->tableName = currentToken().value;
    advance();
    
    // Опциональный список колонок
    if (check(TokenType::LPAREN)) {
        advance();
        do {
            if (!check(TokenType::IDENTIFIER)) {
                throw ParserException("Expected column name", currentToken());
            }
            stmt->columns.push_back(currentToken().value);
            advance();
            
            if (check(TokenType::COMMA)) {
                advance();
            } else {
                break;
            }
        } while (!isEnd());
        expect(TokenType::RPAREN, "Expected ')' after column list");
    }
    
    // VALUES
    expect(TokenType::VALUES, "Expected VALUES keyword");
    stmt->values = parseValuesList();
    
    return stmt;
}

std::vector<UpdateAssignment> Parser::parseSetClause() {
    std::vector<UpdateAssignment> assignments;
    
    expect(TokenType::SET, "Expected SET keyword");
    
    do {
        UpdateAssignment assign;
        
        expect(TokenType::IDENTIFIER, "Expected column name in SET clause");
        assign.column = currentToken().value;
        advance();
        
        expect(TokenType::EQUALS, "Expected '=' in SET clause");
        
        if (check(TokenType::STRING_LITERAL)) {
            assign.value = currentToken().value;
            assign.isString = true;
            advance();
        } else if (check(TokenType::INTEGER_LITERAL)) {
            assign.value = currentToken().value;
            assign.isString = false;
            advance();
        } else {
            throw ParserException("Expected value in SET clause", currentToken());
        }
        
        assignments.push_back(assign);
        
        if (check(TokenType::COMMA)) {
            advance();
        } else {
            break;
        }
    } while (!isEnd());
    
    return assignments;
}

SQLCommand Parser::parseUpdate() {
    auto stmt = std::make_unique<UpdateStmt>();
    
    advance(); // пропускаем UPDATE
    
    if (!check(TokenType::IDENTIFIER)) {
        throw ParserException("Expected table name after UPDATE", currentToken());
    }
    stmt->tableName = currentToken().value;
    advance();
    
    // SET
    stmt->assignments = parseSetClause();
    
    // Опциональное WHERE
    if (check(TokenType::WHERE)) {
        stmt->conditions = parseWhereClause();
    }
    
    return stmt;
}

SQLCommand Parser::parseDelete() {
    auto stmt = std::make_unique<DeleteStmt>();
    
    advance(); // пропускаем DELETE
    expect(TokenType::FROM, "Expected FROM after DELETE");
    
    expect(TokenType::IDENTIFIER, "Expected table name after FROM");
    stmt->tableName = currentToken().value;
    advance();
    
    // Опциональное WHERE
    if (check(TokenType::WHERE)) {
        stmt->conditions = parseWhereClause();
    }
    
    return stmt;
}

std::vector<ColumnDef> Parser::parseColumnDefs() {
    std::vector<ColumnDef> columns;
    
    do {
        ColumnDef col;
        col.size = 0;
        
        expect(TokenType::IDENTIFIER, "Expected column name");
        col.name = currentToken().value;
        advance();
        
        // Тип данных
        if (check(TokenType::IDENTIFIER)) {
            std::string typeName = currentToken().value;
            for (char& c : typeName) c = std::toupper(static_cast<unsigned char>(c));
            col.type = typeName;
            advance();
            
            // Опциональный размер TEXT(n)
            if (typeName == "TEXT" && check(TokenType::LPAREN)) {
                advance();
                expect(TokenType::INTEGER_LITERAL, "Expected size in TEXT(n)");
                col.size = std::stol(currentToken().value);
                advance();
                expect(TokenType::RPAREN, "Expected ')' after TEXT(n)");
            }
        } else {
            throw ParserException("Expected data type", currentToken());
        }
        
        columns.push_back(col);
        
        if (check(TokenType::COMMA)) {
            advance();
        } else {
            break;
        }
    } while (!isEnd());
    
    return columns;
}

SQLCommand Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStmt>();
    
    advance(); // пропускаем CREATE
    expect(TokenType::TABLE, "Expected TABLE after CREATE");
    
    expect(TokenType::IDENTIFIER, "Expected table name");
    stmt->tableName = currentToken().value;
    advance();
    
    expect(TokenType::LPAREN, "Expected '(' after table name");
    stmt->columns = parseColumnDefs();
    expect(TokenType::RPAREN, "Expected ')' after column definitions");
    
    return stmt;
}

SQLCommand Parser::parseDropTable() {
    auto stmt = std::make_unique<DropTableStmt>();
    
    advance(); // пропускаем DROP
    expect(TokenType::TABLE, "Expected TABLE after DROP");
    
    expect(TokenType::IDENTIFIER, "Expected table name");
    stmt->tableName = currentToken().value;
    advance();
    
    return stmt;
}

// Функция для удобного парсинга строки
SQLCommand parseSQL(const std::string& sql) {
    Lexer lexer(sql);
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    return parser.parse();
}

}
