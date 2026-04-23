#include "database.h"
#include <cstring>
#include <fstream>

DatabaseTable::DatabaseTable(const std::string& name) 
    : tableName(name), handle(nullptr), isOpen(false) {}

DatabaseTable::~DatabaseTable() { close(); }

bool DatabaseTable::create(const std::vector<FieldDefinition>& fieldsDef) {
    if (isOpen) close();
    schema = fieldsDef;
    TableStruct tableStruct;
    tableStruct.numOfFields = schema.size();
    std::vector<FieldDef> cFields(schema.size());
    for (size_t i = 0; i < schema.size(); i++) {
        strncpy(cFields[i].name, schema[i].name.c_str(), MaxFieldNameLen - 1);
        cFields[i].name[MaxFieldNameLen - 1] = '\0';
        switch (schema[i].type) {
            case SQLType::INTEGER:
                cFields[i].type = Long;
                cFields[i].len = sizeof(long);
                break;
            case SQLType::VARCHAR:
                cFields[i].type = Text;
                cFields[i].len = schema[i].length;
                break;
            default:
                cFields[i].type = Long;
                cFields[i].len = sizeof(long);
        }
    }
    tableStruct.fieldsDef = cFields.data();
    std::string fileName = tableName + ".dat";
    Errors err = createTable(const_cast<char*>(fileName.c_str()), &tableStruct);
    if (err != OK) return false;
    return open();
}

bool DatabaseTable::open() {
    if (isOpen) return true;
    std::string fileName = tableName + ".dat";
    Errors err = ::openTable(const_cast<char*>(fileName.c_str()), &handle);
    if (err != OK) return false;
    unsigned numFields;
    if (getFieldsNum(handle, &numFields) == OK) {
        schema.clear();
        for (unsigned i = 0; i < numFields; i++) {
            char* fieldName; FieldType type; unsigned len;
            if (getFieldName(handle, i, &fieldName) == OK &&
                getFieldType(handle, fieldName, &type) == OK &&
                getFieldLen(handle, fieldName, &len) == OK) {
                FieldDefinition def;
                def.name = fieldName; def.length = len;
                def.type = (type == Long) ? SQLType::INTEGER : SQLType::VARCHAR;
                schema.push_back(def);
            }
        }
    }
    isOpen = true;
    return true;
}

void DatabaseTable::close() {
    if (handle) { ::closeTable(handle); handle = nullptr; }
    isOpen = false;
}

bool DatabaseTable::drop() {
    close();
    std::string fileName = tableName + ".dat";
    return (deleteTable(const_cast<char*>(fileName.c_str())) == OK);
}

bool DatabaseTable::insert(const std::map<std::string, SQLValue>& values) {
    if (!isOpen && !open()) return false;
    Errors err = createNew(handle);
    if (err != OK) return false;
    for (const auto& field : schema) {
        auto it = values.find(field.name);
        if (it != values.end()) {
            if (field.type == SQLType::INTEGER) {
                long val = std::holds_alternative<int>(it->second) ? std::get<int>(it->second) : 0;
                err = putLongNew(handle, const_cast<char*>(field.name.c_str()), val);
            } else {
                std::string val = std::holds_alternative<std::string>(it->second) ? std::get<std::string>(it->second) : "";
                err = putTextNew(handle, const_cast<char*>(field.name.c_str()), const_cast<char*>(val.c_str()));
            }
            if (err != OK) return false;
        }
    }
    return (insertzNew(handle) == OK);
}

SQLResult DatabaseTable::select(const std::vector<std::string>& columns, const std::vector<WhereCondition>& conditions) {
    SQLResult result;
    if (!isOpen && !open()) return SQLResult::error("Cannot open table");
    std::vector<std::string> selectedColumns;
    for (const auto& col : columns) {
        if (col == "*") {
            for (const auto& field : schema) selectedColumns.push_back(field.name);
            break;
        }
        selectedColumns.push_back(col);
    }
    if (selectedColumns.empty()) return SQLResult::error("No columns selected");
    result.columns = selectedColumns;
    Errors err = moveFirst(handle);
    if (err != OK) return result;
    while (!afterLast(handle)) {
        bool match = true;
        for (const auto& cond : conditions) { if (!checkCondition(handle, cond)) { match = false; break; } }
        if (match) {
            std::map<std::string, SQLValue> row;
            for (const auto& col : selectedColumns) {
                for (const auto& field : schema) {
                    if (field.name == col) {
                        if (field.type == SQLType::INTEGER) {
                            long val;
                            if (getLong(handle, const_cast<char*>(col.c_str()), &val) == OK) row[col] = static_cast<int>(val);
                        } else {
                            char* val;
                            if (getText(handle, const_cast<char*>(col.c_str()), &val) == OK) row[col] = std::string(val ? val : "");
                        }
                        break;
                    }
                }
            }
            result.rows.push_back(row);
        }
        err = moveNext(handle);
        if (err != OK) break;
    }
    return result;
}

int DatabaseTable::update(const std::map<std::string, SQLValue>& values, const std::vector<WhereCondition>& conditions) {
    if (!isOpen && !open()) return 0;
    int updatedCount = 0;
    Errors err = moveFirst(handle);
    if (err != OK) return 0;
    while (!afterLast(handle)) {
        bool match = true;
        for (const auto& cond : conditions) { if (!checkCondition(handle, cond)) { match = false; break; } }
        if (match) {
            err = startEdit(handle);
            if (err == OK) {
                for (const auto& kv : values) {
                    for (const auto& field : schema) {
                        if (field.name == kv.first) {
                            if (field.type == SQLType::INTEGER) {
                                long val = std::holds_alternative<int>(kv.second) ? std::get<int>(kv.second) : 0;
                                putLong(handle, const_cast<char*>(field.name.c_str()), val);
                            } else {
                                std::string val = std::holds_alternative<std::string>(kv.second) ? std::get<std::string>(kv.second) : "";
                                putText(handle, const_cast<char*>(field.name.c_str()), const_cast<char*>(val.c_str()));
                            }
                            break;
                        }
                    }
                }
                err = finishEdit(handle);
                if (err == OK) updatedCount++;
            }
        }
        err = moveNext(handle);
        if (err != OK) break;
    }
    return updatedCount;
}

int DatabaseTable::deleteRecords(const std::vector<WhereCondition>& conditions) {
    if (!isOpen && !open()) return 0;
    int deletedCount = 0;
    Errors err = moveFirst(handle);
    if (err != OK) return 0;
    while (!afterLast(handle)) {
        bool match = true;
        for (const auto& cond : conditions) { if (!checkCondition(handle, cond)) { match = false; break; } }
        if (match) { err = deleteRec(handle); if (err == OK) deletedCount++; }
        err = moveNext(handle);
        if (err != OK) break;
    }
    return deletedCount;
}

std::vector<FieldDefinition> DatabaseTable::getSchema() const { return schema; }
bool DatabaseTable::exists() const { std::ifstream f(tableName + ".dat"); return f.good(); }

bool DatabaseTable::checkCondition(THandle h, const WhereCondition& cond) {
    SQLValue fieldValue; bool found = false;
    for (const auto& field : schema) {
        if (field.name == cond.column) {
            found = true;
            if (field.type == SQLType::INTEGER) {
                long val;
                if (getLong(h, const_cast<char*>(cond.column.c_str()), &val) == OK) fieldValue = static_cast<int>(val);
            } else {
                char* val;
                if (getText(h, const_cast<char*>(cond.column.c_str()), &val) == OK) fieldValue = std::string(val ? val : "");
            }
            break;
        }
    }
    if (!found) return false;
    return compareValues(fieldValue, cond.op, cond.value);
}

bool DatabaseTable::compareValues(const SQLValue& left, const std::string& op, const SQLValue& right) {
    if (std::holds_alternative<std::nullptr_t>(left) || std::holds_alternative<std::nullptr_t>(right)) {
        bool lNull = std::holds_alternative<std::nullptr_t>(left), rNull = std::holds_alternative<std::nullptr_t>(right);
        return (op == "=") ? (lNull && rNull) : ((op == "<>") ? !(lNull && rNull) : false);
    }
    if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
        int l = std::get<int>(left), r = std::get<int>(right);
        if (op == "=") return l == r; if (op == "<>" || op == "!=") return l != r;
        if (op == "<") return l < r; if (op == ">") return l > r;
        if (op == "<=") return l <= r; if (op == ">=") return l >= r;
    }
    if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
        const std::string& l = std::get<std::string>(left), &r = std::get<std::string>(right);
        if (op == "=") return l == r; if (op == "<>" || op == "!=") return l != r;
        if (op == "<") return l < r; if (op == ">") return l > r;
        if (op == "<=") return l <= r; if (op == ">=") return l >= r;
        if (op == "LIKE") {
            if (r.front() == '%' && r.back() == '%') return l.find(r.substr(1, r.size()-2)) != std::string::npos;
            if (r.front() == '%') return l.size() >= r.size()-1 && l.compare(l.size()-(r.size()-1), r.size()-1, r.substr(1)) == 0;
            if (r.back() == '%') return l.compare(0, r.size()-1, r.substr(0, r.size()-1)) == 0;
            return l == r;
        }
    }
    return false;
}

DatabaseManager::DatabaseManager() {}
DatabaseManager::~DatabaseManager() { tables.clear(); }

SQLResult DatabaseManager::createTable(const std::string& tableName, const std::vector<FieldDefinition>& fields) {
    if (tables.find(tableName) != tables.end()) return SQLResult::error("Table already exists: " + tableName);
    auto table = std::make_unique<DatabaseTable>(tableName);
    if (!table->create(fields)) return SQLResult::error("Failed to create table: " + tableName);
    tables[tableName] = std::move(table);
    return SQLResult();
}

SQLResult DatabaseManager::dropTable(const std::string& tableName) {
    auto it = tables.find(tableName);
    if (it != tables.end()) { it->second->drop(); tables.erase(it); }
    else { DatabaseTable temp(tableName); if (!temp.drop()) return SQLResult::error("Failed to drop table: " + tableName); }
    return SQLResult();
}

SQLResult DatabaseManager::executeSelect(std::shared_ptr<SelectStatement> stmt) {
    if (!stmt) return SQLResult::error("Invalid statement");
    auto it = tables.find(stmt->table);
    if (it == tables.end()) {
        auto table = std::make_unique<DatabaseTable>(stmt->table);
        if (!table->exists()) return SQLResult::error("Table not found: " + stmt->table);
        if (!table->open()) return SQLResult::error("Cannot open table: " + stmt->table);
        return table->select(stmt->columns, stmt->whereConditions);
    }
    return it->second->select(stmt->columns, stmt->whereConditions);
}

SQLResult DatabaseManager::executeInsert(std::shared_ptr<InsertStatement> stmt) {
    if (!stmt) return SQLResult::error("Invalid statement");
    auto it = tables.find(stmt->table);
    std::map<std::string, SQLValue> values;
    for (size_t i = 0; i < stmt->columns.size() && i < stmt->values.size(); i++) values[stmt->columns[i]] = stmt->values[i];
    if (it == tables.end()) {
        auto table = std::make_unique<DatabaseTable>(stmt->table);
        if (!table->exists()) return SQLResult::error("Table not found: " + stmt->table);
        if (!table->open()) return SQLResult::error("Cannot open table: " + stmt->table);
        if (!table->insert(values)) return SQLResult::error("Failed to insert record");
        tables[stmt->table] = std::move(table);
        return SQLResult();
    }
    if (!it->second->insert(values)) return SQLResult::error("Failed to insert record");
    return SQLResult();
}

SQLResult DatabaseManager::executeUpdate(std::shared_ptr<UpdateStatement> stmt) {
    if (!stmt) return SQLResult::error("Invalid statement");
    auto it = tables.find(stmt->table);
    if (it == tables.end()) return SQLResult::error("Table not found: " + stmt->table);
    int count = it->second->update(stmt->setValues, stmt->whereConditions);
    SQLResult result; result.rows.push_back({{"rows_affected", count}}); return result;
}

SQLResult DatabaseManager::executeDelete(std::shared_ptr<DeleteStatement> stmt) {
    if (!stmt) return SQLResult::error("Invalid statement");
    auto it = tables.find(stmt->table);
    if (it == tables.end()) return SQLResult::error("Table not found: " + stmt->table);
    int count = it->second->deleteRecords(stmt->whereConditions);
    SQLResult result; result.rows.push_back({{"rows_affected", count}}); return result;
}

SQLResult DatabaseManager::execute(const std::string& sql) {
    auto stmt = parser.parse(sql);
    if (!stmt) return SQLResult::error(parser.getLastError());
    switch (stmt->getType()) {
        case SQLCommandType::SELECT: return executeSelect(std::dynamic_pointer_cast<SelectStatement>(stmt));
        case SQLCommandType::INSERT: return executeInsert(std::dynamic_pointer_cast<InsertStatement>(stmt));
        case SQLCommandType::UPDATE: return executeUpdate(std::dynamic_pointer_cast<UpdateStatement>(stmt));
        case SQLCommandType::DELETE: return executeDelete(std::dynamic_pointer_cast<DeleteStatement>(stmt));
        case SQLCommandType::CREATE_TABLE: { auto s = std::dynamic_pointer_cast<CreateTableStatement>(stmt); return createTable(s->tableName, s->fields); }
        case SQLCommandType::DROP_TABLE: { auto s = std::dynamic_pointer_cast<DropTableStatement>(stmt); return dropTable(s->tableName); }
        default: return SQLResult::error("Unknown SQL command");
    }
}

DatabaseTable* DatabaseManager::getOrCreateTable(const std::string& tableName) {
    auto it = tables.find(tableName);
    if (it != tables.end()) return it->second.get();
    auto table = std::make_unique<DatabaseTable>(tableName);
    if (table->exists() && table->open()) { DatabaseTable* ptr = table.get(); tables[tableName] = std::move(table); return ptr; }
    return nullptr;
}
