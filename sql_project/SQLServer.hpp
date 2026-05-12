#pragma once
#include "Table.hpp"
#include "SQLParser.hpp"
#include <unordered_map>
#include <memory>
#include <functional>
#include <stdexcept>
#include <cstring>

class SQLExecutionException : public std::runtime_error {
public:
    explicit SQLExecutionException(const std::string& m) : std::runtime_error("Execution error: " + m) {}
};

// ─────────────────────────────────────────────────────────────
//  Condition evaluator
// ─────────────────────────────────────────────────────────────
class CondEvaluator {
public:
    static bool eval(const std::shared_ptr<CondExpr>& expr, const Row& row) {
        if (!expr) return true;
        switch (expr->kind) {
            case CondExpr::Kind::And: return eval(expr->left,row) && eval(expr->right,row);
            case CondExpr::Kind::Or:  return eval(expr->left,row) || eval(expr->right,row);
            case CondExpr::Kind::Not: return !eval(expr->child,row);
            case CondExpr::Kind::Cmp: return evalCmp(*expr->cmp, row);
        }
        return false;
    }

private:
    static bool evalCmp(const Comparison& cmp, const Row& row) {
        const Value* v = row.get(cmp.column);
        if (!v) throw SQLExecutionException("Column '" + cmp.column + "' not found in row");

        if (v->type == Long) {
            if (!std::holds_alternative<long>(cmp.value))
                throw SQLExecutionException("Type mismatch: column '" + cmp.column + "' is LONG");
            long rv = std::get<long>(cmp.value);
            long lv = v->longVal;
            return compare(lv, rv, cmp.op);
        } else {
            std::string rv;
            if (std::holds_alternative<long>(cmp.value))
                rv = std::to_string(std::get<long>(cmp.value));
            else
                rv = std::get<std::string>(cmp.value);
            return compare(v->textVal, rv, cmp.op);
        }
    }

    template<typename T>
    static bool compare(const T& a, const T& b, const std::string& op) {
        if (op=="=")  return a==b;
        if (op=="<>") return a!=b;
        if (op=="<")  return a<b;
        if (op==">")  return a>b;
        if (op=="<=") return a<=b;
        if (op==">=") return a>=b;
        throw SQLExecutionException("Unknown operator: " + op);
    }
};

// ─────────────────────────────────────────────────────────────
//  SQLServer – receives SQL string, returns ResultSet
// ─────────────────────────────────────────────────────────────
class SQLServer {
public:
    // Execute a SQL string; throws on error
    ResultSet execute(const std::string& sql) {
        SQLParser parser(sql);
        Statement stmt = parser.parse();
        return std::visit([this](auto&& s){ return dispatch(s); }, stmt);
    }

private:
    // ── visitors ─────────────────────────────────────────────
    ResultSet dispatch(const SelectStmt& s)      { return execSelect(s); }
    ResultSet dispatch(const InsertStmt& s)      { return execInsert(s); }
    ResultSet dispatch(const UpdateStmt& s)      { return execUpdate(s); }
    ResultSet dispatch(const DeleteStmt& s)      { return execDelete(s); }
    ResultSet dispatch(const CreateTableStmt& s) { return execCreate(s); }
    ResultSet dispatch(const DropTableStmt& s)   { return execDrop(s); }

    // ── SELECT ───────────────────────────────────────────────
    ResultSet execSelect(const SelectStmt& stmt) {
        Table tbl(stmt.table);
        auto allCols = tbl.columnNames();

        std::vector<std::string> selCols = stmt.cols.star
            ? allCols : stmt.cols.names;

        // validate requested columns
        for (auto& c : selCols) {
            bool found = std::find(allCols.begin(),allCols.end(),c) != allCols.end();
            if (!found) throw SQLExecutionException("Unknown column: " + c);
        }

        ResultSet rs;
        rs.columns = selCols;

        if (!tbl.first()) return rs; // empty table

        do {
            Row full = tbl.currentRow();
            if (!CondEvaluator::eval(stmt.where, full)) continue;

            Row out;
            for (auto& c : selCols) out.set(c, *full.get(c));
            rs.rows.push_back(out);

        } while (tbl.next());

        rs.message = std::to_string(rs.rows.size()) + " row(s) selected";
        return rs;
    }

    // ── INSERT ───────────────────────────────────────────────
    ResultSet execInsert(const InsertStmt& stmt) {
        Table tbl(stmt.table);
        auto allCols = tbl.columnNames();

        Row row;
        for (size_t i=0; i<stmt.columns.size(); i++) {
            const std::string& col = stmt.columns[i];
            if (std::find(allCols.begin(),allCols.end(),col)==allCols.end())
                throw SQLExecutionException("Unknown column: " + col);

            enum FieldType ft = tbl.columnType(col);
            const LiteralValue& lv = stmt.values[i];

            if (ft == Long) {
                if (!std::holds_alternative<long>(lv))
                    throw SQLExecutionException("Column '" + col + "' expects LONG value");
                row.set(col, Value::fromLong(std::get<long>(lv)));
            } else {
                std::string sv = std::holds_alternative<long>(lv)
                    ? std::to_string(std::get<long>(lv))
                    : std::get<std::string>(lv);
                row.set(col, Value::fromText(sv));
            }
        }

        // fill missing columns with defaults
        for (auto& c : allCols) {
            if (!row.get(c)) {
                if (tbl.columnType(c)==Long) row.set(c, Value::fromLong(0));
                else                          row.set(c, Value::fromText(""));
            }
        }

        tbl.insert(row);
        ResultSet rs;
        rs.affected = 1;
        rs.message = "1 row inserted";
        return rs;
    }

    // ── UPDATE ───────────────────────────────────────────────
    ResultSet execUpdate(const UpdateStmt& stmt) {
        Table tbl(stmt.table);
        auto allCols = tbl.columnNames();

        // Validate assignment columns & build typed values
        std::vector<std::string> cols;
        std::vector<Value> vals;
        for (auto& a : stmt.assignments) {
            if (std::find(allCols.begin(),allCols.end(),a.column)==allCols.end())
                throw SQLExecutionException("Unknown column: " + a.column);
            enum FieldType ft = tbl.columnType(a.column);
            cols.push_back(a.column);
            if (ft==Long) {
                long v = std::holds_alternative<long>(a.value)
                    ? std::get<long>(a.value)
                    : std::stol(std::get<std::string>(a.value));
                vals.push_back(Value::fromLong(v));
            } else {
                std::string v = std::holds_alternative<long>(a.value)
                    ? std::to_string(std::get<long>(a.value))
                    : std::get<std::string>(a.value);
                vals.push_back(Value::fromText(v));
            }
        }

        int count = 0;
        if (!tbl.first()) { ResultSet rs; rs.message="0 rows updated"; return rs; }

        // Collect positions to update (avoid cursor invalidation)
        // We iterate, flag matches, then update in-place via startEdit/finishEdit
        // Since Table iterates forward and deleteRec/finishEdit don't reposition,
        // we can update current record safely.
        do {
            Row full = tbl.currentRow();
            if (!CondEvaluator::eval(stmt.where, full)) continue;
            tbl.updateCurrent(cols, vals);
            count++;
        } while (tbl.next());

        ResultSet rs;
        rs.affected = count;
        rs.message = std::to_string(count) + " row(s) updated";
        return rs;
    }

    // ── DELETE ───────────────────────────────────────────────
    ResultSet execDelete(const DeleteStmt& stmt) {
        Table tbl(stmt.table);
        int count = 0;

        if (!tbl.first()) { ResultSet rs; rs.message="0 rows deleted"; return rs; }

        // Collect all rows matching condition, then delete
        // deleteRec + moveNext works correctly per the C driver semantics:
        // after deleteRec the cursor logically moves; we call next manually.
        bool hasMore = true;
        while (hasMore) {
            Row full = tbl.currentRow();
            bool matches = CondEvaluator::eval(stmt.where, full);
            if (matches) {
                tbl.deleteCurrent();
                count++;
                // after delete, try to get next
                hasMore = tbl.next();
            } else {
                hasMore = tbl.next();
            }
        }

        ResultSet rs;
        rs.affected = count;
        rs.message = std::to_string(count) + " row(s) deleted";
        return rs;
    }

    // ── CREATE TABLE ─────────────────────────────────────────
    ResultSet execCreate(const CreateTableStmt& stmt) {
        std::vector<ColumnDef> cols;
        for (auto& c : stmt.cols) {
            if (c.type=="LONG")
                cols.push_back(ColumnDef::longCol(c.name));
            else {
                if (c.len<=0) throw SQLExecutionException("TEXT column '" + c.name + "' needs length > 0");
                cols.push_back(ColumnDef::textCol(c.name, c.len));
            }
        }
        Table::create(stmt.table, cols);
        ResultSet rs;
        rs.message = "Table '" + stmt.table + "' created";
        return rs;
    }

    // ── DROP TABLE ───────────────────────────────────────────
    ResultSet execDrop(const DropTableStmt& stmt) {
        Table::drop(stmt.table);
        ResultSet rs;
        rs.message = "Table '" + stmt.table + "' dropped";
        return rs;
    }
};
