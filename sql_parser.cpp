#include "sql_parser.h"
#include <cctype>
#include <algorithm>
#include <sstream>

SQLParser::SQLParser() : pos(0) {}

SQLParser::~SQLParser() {}

std::shared_ptr<SQLStatement> SQLParser::parse(const std::string& sql) {
    input = sql;
    pos = 0;
    lastError.clear();
    
    skipWhitespace();
    if (input.empty()) {
        lastError = "Empty SQL statement";
        return nullptr;
    }
    
    std::string keyword = readKeyword();
    
    // Преобразуем к верхнему регистру для сравнения
    std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::toupper);
    
    if (keyword == "SELECT") {
        return parseSelect();
    } else if (keyword == "INSERT") {
        return parseInsert();
    } else if (keyword == "UPDATE") {
        return parseUpdate();
    } else if (keyword == "DELETE") {
        return parseDelete();
    } else if (keyword == "CREATE") {
        skipWhitespace();
        std::string next = readKeyword();
        std::transform(next.begin(), next.end(), next.begin(), ::toupper);
        if (next == "TABLE") {
            return parseCreateTable();
        } else {
            lastError = "Unknown CREATE command: " + next;
            return nullptr;
        }
    } else if (keyword == "DROP") {
        skipWhitespace();
        std::string next = readKeyword();
        std::transform(next.begin(), next.end(), next.begin(), ::toupper);
        if (next == "TABLE") {
            return parseDropTable();
        } else {
            lastError = "Unknown DROP command: " + next;
            return nullptr;
        }
    } else {
        lastError = "Unknown SQL command: " + keyword;
        return nullptr;
    }
}

void SQLParser::skipWhitespace() {
    while (pos < input.length() && std::isspace(input[pos])) {
        pos++;
    }
}

std::string SQLParser::readKeyword() {
    skipWhitespace();
    std::string result;
    while (pos < input.length() && (std::isalpha(input[pos]) || input[pos] == '_')) {
        result += input[pos];
        pos++;
    }
    return result;
}

std::string SQLParser::readIdentifier() {
    skipWhitespace();
    std::string result;
    
    // Проверка на кавычки
    if (pos < input.length() && (input[pos] == '"' || input[pos] == '`' || input[pos] == '[')) {
        char quote = input[pos];
        char endQuote = (quote == '[') ? ']' : quote;
        pos++;
        while (pos < input.length() && input[pos] != endQuote) {
            result += input[pos];
            pos++;
        }
        if (pos < input.length()) pos++; // пропускаем закрывающую кавычку
    } else {
        while (pos < input.length() && (std::isalnum(input[pos]) || input[pos] == '_')) {
            result += input[pos];
            pos++;
        }
    }
    return result;
}

std::string SQLParser::readQuotedString() {
    skipWhitespace();
    std::string result;
    
    if (pos >= input.length() || input[pos] != '\'') {
        lastError = "Expected string literal starting with '";
        return "";
    }
    
    pos++; // пропускаем открывающую кавычку
    while (pos < input.length() && input[pos] != '\'') {
        result += input[pos];
        pos++;
    }
    
    if (pos < input.length()) pos++; // пропускаем закрывающую кавычку
    return result;
}

SQLValue SQLParser::readValue() {
    skipWhitespace();
    
    if (pos >= input.length()) {
        lastError = "Unexpected end of input while reading value";
        return nullptr;
    }
    
    // Строковое значение в кавычках
    if (input[pos] == '\'') {
        return readQuotedString();
    }
    
    // Числовое значение или NULL
    std::string token;
    while (pos < input.length() && (std::isalnum(input[pos]) || input[pos] == '.' || input[pos] == '-')) {
        token += input[pos];
        pos++;
    }
    
    std::transform(token.begin(), token.end(), token.begin(), ::toupper);
    if (token == "NULL") {
        return nullptr;
    }
    
    // Пытаемся распарсить как integer
    try {
        return std::stoi(token);
    } catch (...) {
        return token;
    }
}

std::string SQLParser::readOperator() {
    skipWhitespace();
    std::string op;
    
    // Двухсимвольные операторы
    if (pos + 1 < input.length()) {
        std::string twoChar = input.substr(pos, 2);
        if (twoChar == "<>" || twoChar == "<=" || twoChar == ">=" || twoChar == "!=") {
            pos += 2;
            return twoChar;
        }
    }
    
    // Односимвольные операторы
    if (pos < input.length() && (input[pos] == '=' || input[pos] == '<' || input[pos] == '>' || 
                                  input[pos] == ',' || input[pos] == '(' || input[pos] == ')' ||
                                  input[pos] == '*')) {
        op += input[pos];
        pos++;
        return op;
    }
    
    // LIKE
    skipWhitespace();
    std::string keyword = readKeyword();
    std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::toupper);
    if (keyword == "LIKE") {
        return "LIKE";
    }
    
    return "";
}

std::shared_ptr<SelectStatement> SQLParser::parseSelect() {
    auto stmt = std::make_shared<SelectStatement>();
    
    // Читаем список колонок
    skipWhitespace();
    while (true) {
        std::string col;
        if (pos < input.length() && input[pos] == '*') {
            col = "*";
            pos++;
        } else {
            col = readIdentifier();
        }
        
        if (col.empty()) {
            lastError = "Expected column name in SELECT";
            return nullptr;
        }
        stmt->columns.push_back(col);
        
        skipWhitespace();
        if (pos >= input.length()) break;
        if (input[pos] != ',') break;
        pos++; // пропускаем запятую
    }
    
    // Ожидаем FROM
    skipWhitespace();
    std::string from = readKeyword();
    std::transform(from.begin(), from.end(), from.begin(), ::toupper);
    if (from != "FROM") {
        lastError = "Expected FROM keyword, got: " + from;
        return nullptr;
    }
    
    // Читаем имя таблицы
    stmt->table = readIdentifier();
    if (stmt->table.empty()) {
        lastError = "Expected table name after FROM";
        return nullptr;
    }
    
    // Проверяем WHERE
    skipWhitespace();
    if (pos < input.length()) {
        std::string whereCheck = readKeyword();
        std::string whereUpper = whereCheck;
        std::transform(whereUpper.begin(), whereUpper.end(), whereUpper.begin(), ::toupper);
        
        if (whereUpper == "WHERE") {
            stmt->hasWhere = true;
            // Возвращаем позицию назад для parseWhereClause
            pos -= whereCheck.length();
            skipWhitespace();
            stmt->whereConditions = parseWhereClause();
        } else {
            // Возвращаем позицию назад
            pos -= whereCheck.length();
        }
    }
    
    return stmt;
}

std::shared_ptr<InsertStatement> SQLParser::parseInsert() {
    auto stmt = std::make_shared<InsertStatement>();
    
    // Ожидаем INTO
    skipWhitespace();
    std::string into = readKeyword();
    std::transform(into.begin(), into.end(), into.begin(), ::toupper);
    if (into != "INTO") {
        lastError = "Expected INTO keyword, got: " + into;
        return nullptr;
    }
    
    // Читаем имя таблицы
    stmt->table = readIdentifier();
    if (stmt->table.empty()) {
        lastError = "Expected table name after INTO";
        return nullptr;
    }
    
    // Ожидаем открывающую скобку
    skipWhitespace();
    if (pos >= input.length() || input[pos] != '(') {
        lastError = "Expected '(' after table name";
        return nullptr;
    }
    pos++;
    
    // Читаем список колонок
    while (true) {
        std::string col = readIdentifier();
        if (col.empty()) {
            lastError = "Expected column name";
            return nullptr;
        }
        stmt->columns.push_back(col);
        
        skipWhitespace();
        if (pos >= input.length()) break;
        if (input[pos] == ')') {
            pos++;
            break;
        }
        if (input[pos] != ',') {
            lastError = "Expected ',' or ')'";
            return nullptr;
        }
        pos++;
    }
    
    // Ожидаем VALUES
    skipWhitespace();
    std::string values = readKeyword();
    std::transform(values.begin(), values.end(), values.begin(), ::toupper);
    if (values != "VALUES") {
        lastError = "Expected VALUES keyword";
        return nullptr;
    }
    
    // Ожидаем открывающую скобку для значений
    skipWhitespace();
    if (pos >= input.length() || input[pos] != '(') {
        lastError = "Expected '(' after VALUES";
        return nullptr;
    }
    pos++;
    
    // Читаем значения
    while (true) {
        SQLValue val = readValue();
        if (std::holds_alternative<std::nullptr_t>(val) && !lastError.empty()) {
            return nullptr;
        }
        stmt->values.push_back(val);
        
        skipWhitespace();
        if (pos >= input.length()) break;
        if (input[pos] == ')') {
            pos++;
            break;
        }
        if (input[pos] != ',') {
            lastError = "Expected ',' or ')'";
            return nullptr;
        }
        pos++;
    }
    
    return stmt;
}

std::shared_ptr<UpdateStatement> SQLParser::parseUpdate() {
    auto stmt = std::make_shared<UpdateStatement>();
    
    // Читаем имя таблицы
    stmt->table = readIdentifier();
    if (stmt->table.empty()) {
        lastError = "Expected table name after UPDATE";
        return nullptr;
    }
    
    // Ожидаем SET
    skipWhitespace();
    std::string set = readKeyword();
    std::transform(set.begin(), set.end(), set.begin(), ::toupper);
    if (set != "SET") {
        lastError = "Expected SET keyword";
        return nullptr;
    }
    
    // Читаем пары column = value
    skipWhitespace();
    while (true) {
        std::string col = readIdentifier();
        if (col.empty()) {
            lastError = "Expected column name in SET";
            return nullptr;
        }
        
        std::string op = readOperator();
        if (op != "=") {
            lastError = "Expected '=' in SET clause";
            return nullptr;
        }
        
        SQLValue val = readValue();
        if (std::holds_alternative<std::nullptr_t>(val) && !lastError.empty()) {
            return nullptr;
        }
        
        stmt->setValues[col] = val;
        
        skipWhitespace();
        if (pos >= input.length()) break;
        if (input[pos] != ',') break;
        pos++;
    }
    
    // Проверяем WHERE
    skipWhitespace();
    if (pos < input.length()) {
        std::string whereCheck = readKeyword();
        std::string whereUpper = whereCheck;
        std::transform(whereUpper.begin(), whereUpper.end(), whereUpper.begin(), ::toupper);
        
        if (whereUpper == "WHERE") {
            stmt->hasWhere = true;
            pos -= whereCheck.length();
            skipWhitespace();
            stmt->whereConditions = parseWhereClause();
        } else {
            pos -= whereCheck.length();
        }
    }
    
    return stmt;
}

std::shared_ptr<DeleteStatement> SQLParser::parseDelete() {
    auto stmt = std::make_shared<DeleteStatement>();
    
    // Ожидаем FROM
    skipWhitespace();
    std::string from = readKeyword();
    std::transform(from.begin(), from.end(), from.begin(), ::toupper);
    if (from != "FROM") {
        lastError = "Expected FROM keyword";
        return nullptr;
    }
    
    // Читаем имя таблицы
    stmt->table = readIdentifier();
    if (stmt->table.empty()) {
        lastError = "Expected table name after FROM";
        return nullptr;
    }
    
    // Проверяем WHERE
    skipWhitespace();
    if (pos < input.length()) {
        std::string whereCheck = readKeyword();
        std::string whereUpper = whereCheck;
        std::transform(whereUpper.begin(), whereUpper.end(), whereUpper.begin(), ::toupper);
        
        if (whereUpper == "WHERE") {
            stmt->hasWhere = true;
            pos -= whereCheck.length();
            skipWhitespace();
            stmt->whereConditions = parseWhereClause();
        } else {
            pos -= whereCheck.length();
        }
    }
    
    return stmt;
}

std::shared_ptr<CreateTableStatement> SQLParser::parseCreateTable() {
    auto stmt = std::make_shared<CreateTableStatement>();
    
    // Читаем имя таблицы
    stmt->tableName = readIdentifier();
    if (stmt->tableName.empty()) {
        lastError = "Expected table name after CREATE TABLE";
        return nullptr;
    }
    
    // Ожидаем открывающую скобку
    skipWhitespace();
    if (pos >= input.length() || input[pos] != '(') {
        lastError = "Expected '(' after table name";
        return nullptr;
    }
    pos++;
    
    // Читаем определения полей
    while (true) {
        FieldDefinition field;
        field.name = readIdentifier();
        if (field.name.empty()) {
            lastError = "Expected field name";
            return nullptr;
        }
        
        skipWhitespace();
        std::string typeStr = readKeyword();
        std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::toupper);
        
        if (typeStr == "INTEGER" || typeStr == "INT") {
            field.type = SQLType::INTEGER;
            field.length = sizeof(int);
        } else if (typeStr == "VARCHAR" || typeStr == "CHAR") {
            field.type = SQLType::VARCHAR;
            skipWhitespace();
            if (pos < input.length() && input[pos] == '(') {
                pos++;
                std::string lenStr;
                while (pos < input.length() && std::isdigit(input[pos])) {
                    lenStr += input[pos];
                    pos++;
                }
                if (pos < input.length() && input[pos] == ')') pos++;
                field.length = std::stoi(lenStr);
            }
        } else {
            field.type = SQLType::UNKNOWN;
        }
        
        stmt->fields.push_back(field);
        
        skipWhitespace();
        if (pos >= input.length()) break;
        if (input[pos] == ')') {
            pos++;
            break;
        }
        if (input[pos] != ',') {
            lastError = "Expected ',' or ')'";
            return nullptr;
        }
        pos++;
    }
    
    return stmt;
}

std::shared_ptr<DropTableStatement> SQLParser::parseDropTable() {
    auto stmt = std::make_shared<DropTableStatement>();
    
    // Читаем имя таблицы
    stmt->tableName = readIdentifier();
    if (stmt->tableName.empty()) {
        lastError = "Expected table name after DROP TABLE";
        return nullptr;
    }
    
    return stmt;
}

std::vector<WhereCondition> SQLParser::parseWhereClause() {
    std::vector<WhereCondition> conditions;
    
    skipWhitespace();
    std::string where = readKeyword();
    std::transform(where.begin(), where.end(), where.begin(), ::toupper);
    if (where != "WHERE") {
        lastError = "Expected WHERE keyword";
        return conditions;
    }
    
    while (true) {
        WhereCondition cond = parseCondition();
        if (!lastError.empty()) {
            return conditions;
        }
        conditions.push_back(cond);
        
        skipWhitespace();
        if (pos >= input.length()) break;
        
        // Проверяем AND
        std::string andCheck = readKeyword();
        std::string andUpper = andCheck;
        std::transform(andUpper.begin(), andUpper.end(), andUpper.begin(), ::toupper);
        if (andUpper != "AND") {
            pos -= andCheck.length();
            break;
        }
    }
    
    return conditions;
}

WhereCondition SQLParser::parseCondition() {
    WhereCondition cond;
    
    cond.column = readIdentifier();
    if (cond.column.empty()) {
        lastError = "Expected column name in WHERE condition";
        return cond;
    }
    
    cond.op = readOperator();
    if (cond.op.empty()) {
        lastError = "Expected operator in WHERE condition";
        return cond;
    }
    
    cond.value = readValue();
    if (std::holds_alternative<std::nullptr_t>(cond.value) && !lastError.empty()) {
        return cond;
    }
    
    return cond;
}
