#include "interpreter.h"
#include <sstream>

namespace SQL {
QueryResult Interpreter::execute(const SQLCommand& cmd) {
    try {
        analyzer_.validate(cmd);
        return std::visit([this](const auto& stmt) {
            using T = std::decay_t<decltype(stmt)>;
            if constexpr (std::is_same_v<T, std::unique_ptr<SelectStmt>>) return execSelect(stmt);
            if constexpr (std::is_same_v<T, std::unique_ptr<InsertStmt>>) return execInsert(stmt);
            if constexpr (std::is_same_v<T, std::unique_ptr<UpdateStmt>>) return execUpdate(stmt);
            if constexpr (std::is_same_v<T, std::unique_ptr<DeleteStmt>>) return execDelete(stmt);
            if constexpr (std::is_same_v<T, std::unique_ptr<CreateTableStmt>>) return execCreate(stmt);
            if constexpr (std::is_same_v<T, std::unique_ptr<DropTableStmt>>) return execDrop(stmt);
        }, cmd);
    } catch (const std::exception& e) {
        return {false, e.what(), {}};
    }
}

QueryResult Interpreter::execSelect(const std::unique_ptr<SelectStmt>& stmt) {
    TableLib::Table table;
    table.open(dbPath_ + "/" + stmt->tableName + ".dat");
    QueryResult res{true, "OK", {}};
    
    table.moveFirst();
    while (!table.isAfterLast()) {
        if (matchesWhere(table, stmt->conditions)) {
            std::vector<std::string> row;
            for (auto& col : stmt->columns) {
                if (col.name == "*") {
                    for (unsigned i = 0; i < table.getFieldCount(); ++i)
                        row.push_back(getValue(table, table.getFieldName(i)));
                } else {
                    row.push_back(getValue(table, col.name));
                }
            }
            res.rows.push_back(row);
        }
        table.moveNext();
    }
    return res;
}

QueryResult Interpreter::execInsert(const std::unique_ptr<InsertStmt>& stmt) {
    TableLib::Table table;
    table.open(dbPath_ + "/" + stmt->tableName + ".dat");
    table.createNew();
    for (size_t i = 0; i < stmt->values.size(); ++i) {
        auto& v = stmt->values[i];
        std::string fieldName = stmt->columns.empty() ? table.getFieldName(i) : stmt->columns[i];
        if (v.isNull) {
            // В модели LONG NULL трактуется как 0, TEXT как пустая строка
            table.putLongNew(fieldName, 0);
        } else if (v.isString) {
            table.putTextNew(fieldName, v.value);
        } else {
            table.putLongNew(fieldName, std::stol(v.value));
        }
    }
    table.insertAtEnd();
    return {true, "1 row inserted", {}};
}

QueryResult Interpreter::execUpdate(const std::unique_ptr<UpdateStmt>& stmt) {
    TableLib::Table table;
    table.open(dbPath_ + "/" + stmt->tableName + ".dat");
    int count = 0;
    table.moveFirst();
    while (!table.isAfterLast()) {
        if (matchesWhere(table, stmt->conditions)) {
            table.startEdit();
            for (auto& assign : stmt->assignments) {
                if (assign.isString) table.putText(assign.column, assign.value);
                else table.putLong(assign.column, std::stol(assign.value));
            }
            table.finishEdit();
            count++;
        }
        table.moveNext();
    }
    return {true, std::to_string(count) + " rows updated", {}};
}

QueryResult Interpreter::execDelete(const std::unique_ptr<DeleteStmt>& stmt) {
    TableLib::Table table;
    table.open(dbPath_ + "/" + stmt->tableName + ".dat");
    int count = 0;
    table.moveFirst();
    while (!table.isAfterLast()) {
        if (matchesWhere(table, stmt->conditions)) {
            table.deleteRecord();
            count++;
        } else {
            table.moveNext();
        }
    }
    return {true, std::to_string(count) + " rows deleted", {}};
}

QueryResult Interpreter::execCreate(const std::unique_ptr<CreateTableStmt>& stmt) {
    std::vector<TableLib::FieldDefinition> fields;
    for (auto& col : stmt->columns) {
        auto type = (col.type == "LONG") ? TableLib::FieldType::Long : TableLib::FieldType::Text;
        fields.emplace_back(col.name, type, col.size > 0 ? col.size : sizeof(long));
    }
    TableLib::Table::create(dbPath_ + "/" + stmt->tableName + ".dat", fields);
    return {true, "Table created", {}};
}

QueryResult Interpreter::execDrop(const std::unique_ptr<DropTableStmt>& stmt) {
    TableLib::Table::drop(dbPath_ + "/" + stmt->tableName + ".dat");
    return {true, "Table dropped", {}};
}

bool Interpreter::matchesWhere(TableLib::Table& table, const std::vector<Condition>& conditions) {
    if (conditions.empty()) return true; // WHERE ALL
    bool result = evalCondition(table, conditions[0]);
    for (size_t i = 1; i < conditions.size(); ++i) {
        bool next = evalCondition(table, conditions[i]);
        if (conditions[i-1].logicalOp == "AND") result = result && next;
        else result = result || next;
    }
    return result;
}

bool Interpreter::evalCondition(TableLib::Table& table, const Condition& cond) {
    std::string left = getValue(table, cond.leftField);
    std::string right = cond.isField ? getValue(table, cond.rightValue) : cond.rightValue;
    
    if (cond.op == "=") return left == right;
    if (cond.op == "!=" || cond.op == "<>") return left != right;
    if (cond.op == "<") return left < right;
    if (cond.op == ">") return left > right;
    if (cond.op == "<=") return left <= right;
    if (cond.op == ">=") return left >= right;
    return false;
}

std::string Interpreter::getValue(TableLib::Table& table, const std::string& field) {
    try {
        return table.getText(field);
    } catch (...) {
        try {
            return std::to_string(table.getLong(field));
        } catch (...) {
            return "NULL";
        }
    }
}
} // namespace SQL
