#ifndef INTERPRETER_H
#define INTERPRETER_H
#include "sql_parser.h"
#include "semantic_analyzer.h"
#include "TableWrapper.h"
#include <string>
#include <vector>

namespace SQL {
struct QueryResult {
    bool success;
    std::string message;
    std::vector<std::vector<std::string>> rows; // Для SELECT
};

class Interpreter {
public:
    explicit Interpreter(const std::string& dbPath = ".") : dbPath_(dbPath), analyzer_(dbPath) {}
    QueryResult execute(const SQLCommand& cmd);
private:
    std::string dbPath_;
    SemanticAnalyzer analyzer_;
    
    QueryResult execSelect(const std::unique_ptr<SelectStmt>& stmt);
    QueryResult execInsert(const std::unique_ptr<InsertStmt>& stmt);
    QueryResult execUpdate(const std::unique_ptr<UpdateStmt>& stmt);
    QueryResult execDelete(const std::unique_ptr<DeleteStmt>& stmt);
    QueryResult execCreate(const std::unique_ptr<CreateTableStmt>& stmt);
    QueryResult execDrop(const std::unique_ptr<DropTableStmt>& stmt);
    
    bool matchesWhere(TableLib::Table& table, const std::vector<Condition>& conditions);
    bool evalCondition(TableLib::Table& table, const Condition& cond);
    std::string getValue(TableLib::Table& table, const std::string& field);
};
} // namespace SQL
#endif
