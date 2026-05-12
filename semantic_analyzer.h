#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H
#include "sql_parser.h"
#include "TableWrapper.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace SQL {
class SemanticException : public std::runtime_error {
public:
    explicit SemanticException(const std::string& msg) : std::runtime_error(msg) {}
};

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(const std::string& dbPath = ".") : dbPath_(dbPath) {}
    
    void validate(const SQLCommand& cmd);

private:
    std::string dbPath_;
    
    // Вспомогательные методы
    TableLib::Table openTableCheck(const std::string& name);
    TableLib::FieldType getFieldType(const std::string& table, const std::string& col);
    void validateCondition(const Condition& cond, const std::string& table);
    void validateAssignment(const UpdateAssignment& assign, const std::string& table);
};
} // namespace SQL
#endif
