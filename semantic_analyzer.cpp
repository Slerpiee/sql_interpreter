#include "semantic_analyzer.h"
#include <algorithm>
#include <filesystem>

namespace SQL {
void SemanticAnalyzer::validate(const SQLCommand& cmd) {
    std::visit([this](const auto& stmt) {
        using T = std::decay_t<decltype(stmt)>;
        if constexpr (std::is_same_v<T, std::unique_ptr<SelectStmt>>) {
            auto t = openTableCheck(stmt->tableName);
            for (auto& col : stmt->columns) {
                if (col.name != "*") getFieldType(stmt->tableName, col.name);
            }
            for (auto& cond : stmt->conditions) validateCondition(cond, stmt->tableName);
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<InsertStmt>>) {
            auto t = openTableCheck(stmt->tableName);
            if (stmt->columns.empty()) {
                auto cnt = t.getFieldCount();
                if (stmt->values.size() != cnt)
                    throw SemanticException("Column count mismatch in INSERT");
            } else {
                for (auto& c : stmt->columns) getFieldType(stmt->tableName, c);
            }
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<UpdateStmt>>) {
            openTableCheck(stmt->tableName);
            for (auto& assign : stmt->assignments) validateAssignment(assign, stmt->tableName);
            for (auto& cond : stmt->conditions) validateCondition(cond, stmt->tableName);
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<DeleteStmt>>) {
            openTableCheck(stmt->tableName);
            for (auto& cond : stmt->conditions) validateCondition(cond, stmt->tableName);
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<CreateTableStmt>>) {
            std::string path = dbPath_ + "/" + stmt->tableName + ".dat";
            if (std::filesystem::exists(path))
                throw SemanticException("Table already exists");
        }
        else if constexpr (std::is_same_v<T, std::unique_ptr<DropTableStmt>>) {
            openTableCheck(stmt->tableName);
        }
    }, cmd);
}

TableLib::Table SemanticAnalyzer::openTableCheck(const std::string& name) {
    TableLib::Table t;
    try {
        t.open(dbPath_ + "/" + name + ".dat");
        return t;
    } catch (...) {
        throw SemanticException("Table '" + name + "' not found");
    }
}

TableLib::FieldType SemanticAnalyzer::getFieldType(const std::string& table, const std::string& col) {
    TableLib::Table t;
    t.open(dbPath_ + "/" + table + ".dat");
    return t.getFieldType(col);
}

void SemanticAnalyzer::validateCondition(const Condition& cond, const std::string& table) {
    if (cond.op == "LIKE") {
        auto type = getFieldType(table, cond.leftField);
        if (type != TableLib::FieldType::Text)
            throw SemanticException("LIKE can only be used with TEXT fields");
    }
    // Проверка типов сравнения упрощена для модели: разрешаем LONG vs LONG, TEXT vs TEXT
}

void SemanticAnalyzer::validateAssignment(const UpdateAssignment& assign, const std::string& table) {
    auto type = getFieldType(table, assign.column);
    if (type == TableLib::FieldType::Long && !assign.isString) {
        try { std::stol(assign.value); }
        catch (...) { throw SemanticException("Invalid LONG value in SET"); }
    }
}
} // namespace SQL
