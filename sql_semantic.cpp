#include "sql_semantic.h"
#include <algorithm>
#include <cctype>

namespace SQL {

// ============================================================================
// TableInfo implementation
// ============================================================================

const FieldInfo* TableInfo::findField(const std::string& fieldName) const {
    for (const auto& field : fields) {
        // Сравнение без учета регистра
        std::string f1 = field.name;
        std::string f2 = fieldName;
        for (char& c : f1) c = std::toupper(static_cast<unsigned char>(c));
        for (char& c : f2) c = std::toupper(static_cast<unsigned char>(c));
        if (f1 == f2) {
            return &field;
        }
    }
    return nullptr;
}

// ============================================================================
// SymbolTable implementation
// ============================================================================

void SymbolTable::addTable(const std::string& tableName, const std::vector<FieldInfo>& fields) {
    std::string name = tableName;
    for (char& c : name) c = std::toupper(static_cast<unsigned char>(c));
    
    TableInfo info;
    info.name = tableName;
    info.fields = fields;
    tables_[name] = info;
}

void SymbolTable::removeTable(const std::string& tableName) {
    std::string name = tableName;
    for (char& c : name) c = std::toupper(static_cast<unsigned char>(c));
    tables_.erase(name);
}

const TableInfo* SymbolTable::getTable(const std::string& tableName) const {
    std::string name = tableName;
    for (char& c : name) c = std::toupper(static_cast<unsigned char>(c));
    
    auto it = tables_.find(name);
    if (it != tables_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool SymbolTable::tableExists(const std::string& tableName) const {
    return getTable(tableName) != nullptr;
}

std::vector<std::string> SymbolTable::getTableNames() const {
    std::vector<std::string> names;
    for (const auto& pair : tables_) {
        names.push_back(pair.second.name);
    }
    return names;
}

// ============================================================================
// SemanticAnalyzer implementation
// ============================================================================

SemanticAnalyzer::SemanticAnalyzer(SymbolTable& symbolTable) 
    : symbolTable_(symbolTable) {}

std::string SemanticAnalyzer::normalizeType(const std::string& type) {
    std::string result = type;
    for (char& c : result) c = std::toupper(static_cast<unsigned char>(c));
    return result;
}

void SemanticAnalyzer::analyze(const SQLCommand& cmd) {
    std::visit([this](const auto& stmt) {
        using T = std::decay_t<decltype(stmt)>;
        
        if constexpr (std::is_same_v<T, std::unique_ptr<SelectStmt>>) {
            analyzeSelect(*stmt);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<InsertStmt>>) {
            analyzeInsert(*stmt);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<UpdateStmt>>) {
            analyzeUpdate(*stmt);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<DeleteStmt>>) {
            analyzeDelete(*stmt);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<CreateTableStmt>>) {
            analyzeCreateTable(*stmt);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<DropTableStmt>>) {
            analyzeDropTable(*stmt);
        }
    }, cmd);
}

void SemanticAnalyzer::analyzeSelect(const SelectStmt& stmt) {
    // Проверка существования таблицы
    const TableInfo* table = symbolTable_.getTable(stmt.tableName);
    if (!table) {
        throw SemanticException("Table '" + stmt.tableName + "' does not exist");
    }
    
    // Проверка колонок в SELECT
    for (const auto& col : stmt.columns) {
        if (col.name == "*") continue;  // * всегда допустим
        
        // Если есть псевдоним таблицы
        if (!col.tableAlias.empty()) {
            if (col.tableAlias != stmt.tableName) {
                throw SemanticException("Unknown table alias '" + col.tableAlias + "'");
            }
        }
        
        // Проверка существования поля
        if (!table->findField(col.name)) {
            throw SemanticException("Column '" + col.name + "' does not exist in table '" + stmt.tableName + "'");
        }
    }
    
    // Проверка условий WHERE
    for (const auto& cond : stmt.conditions) {
        analyzeCondition(cond, *table);
    }
}

void SemanticAnalyzer::analyzeInsert(const InsertStmt& stmt) {
    // Проверка существования таблицы
    const TableInfo* table = symbolTable_.getTable(stmt.tableName);
    if (!table) {
        throw SemanticException("Table '" + stmt.tableName + "' does not exist");
    }
    
    // Если указаны колонки, проверяем их существование и порядок
    if (!stmt.columns.empty()) {
        for (const auto& colName : stmt.columns) {
            if (!table->findField(colName)) {
                throw SemanticException("Column '" + colName + "' does not exist in table '" + stmt.tableName + "'");
            }
        }
        
        // Проверка количества значений
        if (stmt.values.size() != stmt.columns.size()) {
            throw SemanticException("Number of values does not match number of columns");
        }
        
        // Проверка типов значений
        for (size_t i = 0; i < stmt.columns.size(); ++i) {
            const FieldInfo* field = table->findField(stmt.columns[i]);
            if (field) {
                checkTypeCompatibility(field->type, stmt.values[i].value, stmt.values[i].isString);
            }
        }
    } else {
        // Если колонки не указаны, проверяем количество значений
        if (stmt.values.size() != table->fields.size()) {
            throw SemanticException("Number of values does not match number of columns in table");
        }
        
        // Проверка типов значений
        for (size_t i = 0; i < table->fields.size(); ++i) {
            checkTypeCompatibility(table->fields[i].type, stmt.values[i].value, stmt.values[i].isString);
        }
    }
}

void SemanticAnalyzer::analyzeUpdate(const UpdateStmt& stmt) {
    // Проверка существования таблицы
    const TableInfo* table = symbolTable_.getTable(stmt.tableName);
    if (!table) {
        throw SemanticException("Table '" + stmt.tableName + "' does not exist");
    }
    
    // Проверка колонок в SET
    for (const auto& assign : stmt.assignments) {
        const FieldInfo* field = table->findField(assign.column);
        if (!field) {
            throw SemanticException("Column '" + assign.column + "' does not exist in table '" + stmt.tableName + "'");
        }
        
        // Проверка типа значения
        checkTypeCompatibility(field->type, assign.value, assign.isString);
    }
    
    // Проверка условий WHERE
    for (const auto& cond : stmt.conditions) {
        analyzeCondition(cond, *table);
    }
}

void SemanticAnalyzer::analyzeDelete(const DeleteStmt& stmt) {
    // Проверка существования таблицы
    const TableInfo* table = symbolTable_.getTable(stmt.tableName);
    if (!table) {
        throw SemanticException("Table '" + stmt.tableName + "' does not exist");
    }
    
    // Проверка условий WHERE
    for (const auto& cond : stmt.conditions) {
        analyzeCondition(cond, *table);
    }
}

void SemanticAnalyzer::analyzeCreateTable(const CreateTableStmt& stmt) {
    // Проверка, что таблица еще не существует
    if (symbolTable_.tableExists(stmt.tableName)) {
        throw SemanticException("Table '" + stmt.tableName + "' already exists");
    }
    
    // Проверка имен колонок на дубликаты
    std::unordered_set<std::string> fieldNames;
    for (const auto& col : stmt.columns) {
        std::string name = col.name;
        for (char& c : name) c = std::toupper(static_cast<unsigned char>(c));
        
        if (fieldNames.count(name)) {
            throw SemanticException("Duplicate column name '" + col.name + "'");
        }
        fieldNames.insert(name);
        
        // Проверка типа данных
        std::string type = normalizeType(col.type);
        if (type != "LONG" && type != "TEXT") {
            throw SemanticException("Invalid data type '" + col.type + "' for column '" + col.name + "'");
        }
        
        // Для TEXT должен быть указан размер
        if (type == "TEXT" && col.size <= 0) {
            throw SemanticException("TEXT column '" + col.name + "' must have a size specification");
        }
    }
    
    // Если все проверки пройдены, регистрируем таблицу
    std::vector<FieldInfo> fields;
    for (const auto& col : stmt.columns) {
        FieldInfo info;
        info.name = col.name;
        info.type = normalizeType(col.type);
        info.size = col.size;
        fields.push_back(info);
    }
    
    symbolTable_.addTable(stmt.tableName, fields);
}

void SemanticAnalyzer::analyzeDropTable(const DropTableStmt& stmt) {
    // Проверка существования таблицы
    if (!symbolTable_.tableExists(stmt.tableName)) {
        throw SemanticException("Table '" + stmt.tableName + "' does not exist");
    }
    
    // Удаляем таблицу из символьной таблицы
    symbolTable_.removeTable(stmt.tableName);
}

void SemanticAnalyzer::analyzeCondition(const Condition& cond, const TableInfo& table) {
    // Проверка существования левого поля
    const FieldInfo* leftField = table.findField(cond.leftField);
    if (!leftField) {
        throw SemanticException("Column '" + cond.leftField + "' does not exist in table '" + table.name + "'");
    }
    
    // Если правая часть - поле, проверяем его существование
    if (cond.isField) {
        const FieldInfo* rightField = table.findField(cond.rightValue);
        if (!rightField) {
            throw SemanticException("Column '" + cond.rightValue + "' does not exist in table '" + table.name + "'");
        }
    } else {
        // Если правая часть - значение, проверяем совместимость типов
        checkTypeCompatibility(leftField->type, cond.rightValue, false);
    }
}

void SemanticAnalyzer::checkTypeCompatibility(const std::string& fieldType, 
                                               const std::string& value, 
                                               bool isStringLiteral) {
    std::string normalizedType = normalizeType(fieldType);
    
    if (normalizedType == "LONG") {
        // Проверяем, что значение - число
        if (value.empty()) {
            throw SemanticException("Empty value for LONG field");
        }
        
        bool isNegative = value[0] == '-';
        size_t start = isNegative ? 1 : 0;
        
        for (size_t i = start; i < value.length(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(value[i]))) {
                throw SemanticException("Value '" + value + "' is not a valid integer for LONG field");
            }
        }
    } else if (normalizedType == "TEXT") {
        // Для TEXT значения должны быть строками
        // В парсере строковые литералы уже обработаны, здесь просто принимаем значение
        // Если isStringLiteral=false, значит это число или идентификатор
        if (!isStringLiteral) {
            // Числа могут быть присвоены TEXT полям (автоматическое преобразование)
            // Идентификаторы (поля) тоже допустимы
        }
    }
}

} // namespace SQL
