#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>

// Максимальная длина имени поля (из библиотеки таблиц)
const unsigned MaxFieldNameLen = 64;

namespace sql {

// ============================================================================
// Базовый класс для всех узлов AST
// ============================================================================
class ASTNode {
public:
    virtual ~ASTNode() = default;
    
    // Виртуальный метод для вывода узла в читаемом виде
    virtual std::string ToString(int indent = 0) const = 0;
    
    // Вспомогательная функция для отступов
    static std::string Indent(int level) {
        return std::string(level * 2, ' ');
    }
};

// ============================================================================
// Узлы для выражений (базовые классы)
// ============================================================================

// Базовый класс для всех выражений
class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

// Базовый класс для лонг-выражений
class LongExpression : public Expression {
public:
    virtual ~LongExpression() = default;
};

// Базовый класс для текстовых выражений
class TextExpression : public Expression {
public:
    virtual ~TextExpression() = default;
};

// ============================================================================
// Литералы и идентификаторы
// ============================================================================

// Целочисленный литерал
class IntegerLiteral : public LongExpression {
public:
    long value;
    
    explicit IntegerLiteral(long val) : value(val) {}
    
    std::string ToString(int indent = 0) const override {
        return Indent(indent) + "IntegerLiteral(" + std::to_string(value) + ")";
    }
};

// Строковый литерал
class StringLiteral : public TextExpression {
public:
    std::string value;
    
    explicit StringLiteral(const std::string& val) : value(val) {}
    
    std::string ToString(int indent = 0) const override {
        return Indent(indent) + "StringLiteral(\"" + value + "\")";
    }
};

// Идентификатор (имя поля или таблицы) - только для LONG выражений
class Identifier : public LongExpression {
public:
    std::string name;
    
    explicit Identifier(const std::string& n) : name(n) {}
    
    std::string ToString(int indent = 0) const override {
        return Indent(indent) + "Identifier(" + name + ")";
    }
};

// ============================================================================
// Арифметические выражения (LONG)
// ============================================================================

// Бинарная операция над LONG выражениями
class LongBinaryOp : public LongExpression {
public:
    enum OpType { ADD, SUB, MUL, DIV, MOD };
    
    OpType op;
    std::unique_ptr<LongExpression> left;
    std::unique_ptr<LongExpression> right;
    
    LongBinaryOp(OpType operation, 
                 std::unique_ptr<LongExpression> l,
                 std::unique_ptr<LongExpression> r)
        : op(operation), left(std::move(l)), right(std::move(r)) {}
    
    std::string ToString(int indent = 0) const override {
        const char* opStr[] = {"+", "-", "*", "/", "%"};
        std::ostringstream oss;
        oss << Indent(indent) << "LongBinaryOp(" << opStr[op] << ")\n";
        oss << left->ToString(indent + 1) << "\n";
        oss << right->ToString(indent + 1);
        return oss.str();
    }
};

// ============================================================================
// Логические выражения
// ============================================================================

// Базовый класс для логических выражений
class LogicalExpression : public Expression {
public:
    virtual ~LogicalExpression() = default;
};

// ============================================================================
// Отношения (сравнения)
// ============================================================================

// <Text-отношение> ::= <Text-выражение> <операция сравнения> <Text-выражение>
// <Long-отношение> ::= <Long-выражение> <операция сравнения> <Long-выражение>
class ComparisonOp : public LogicalExpression {
public:
    enum OpType { EQ, NE, LT, LE, GT, GE };
    
    OpType op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    
    // Флаг, указывающий тип выражений (для проверки типов на этапе интерпретации)
    bool isTextComparison;
    
    ComparisonOp(OpType operation,
                 std::unique_ptr<Expression> l,
                 std::unique_ptr<Expression> r,
                 bool textCmp = false)
        : op(operation), left(std::move(l)), right(std::move(r)), 
          isTextComparison(textCmp) {}
    
    std::string ToString(int indent = 0) const override {
        const char* opStr[] = {"=", "!=", "<", "<=", ">", ">="};
        std::ostringstream oss;
        oss << ASTNode::Indent(indent) << "ComparisonOp(" << opStr[op] << ")\n";
        oss << left->ToString(indent + 1) << "\n";
        oss << right->ToString(indent + 1);
        return oss.str();
    }
};

// NOT операция
class LogicalNot : public LogicalExpression {
public:
    std::unique_ptr<LogicalExpression> operand;
    
    explicit LogicalNot(std::unique_ptr<LogicalExpression> op)
        : operand(std::move(op)) {}
    
    std::string ToString(int indent = 0) const override {
        std::ostringstream oss;
        oss << Indent(indent) << "LogicalNot\n";
        oss << operand->ToString(indent + 1);
        return oss.str();
    }
};

// AND операция
class LogicalAnd : public LogicalExpression {
public:
    std::unique_ptr<LogicalExpression> left;
    std::unique_ptr<LogicalExpression> right;
    
    LogicalAnd(std::unique_ptr<LogicalExpression> l,
               std::unique_ptr<LogicalExpression> r)
        : left(std::move(l)), right(std::move(r)) {}
    
    std::string ToString(int indent = 0) const override {
        std::ostringstream oss;
        oss << Indent(indent) << "LogicalAnd\n";
        oss << left->ToString(indent + 1) << "\n";
        oss << right->ToString(indent + 1);
        return oss.str();
    }
};

// OR операция
class LogicalOr : public LogicalExpression {
public:
    std::unique_ptr<LogicalExpression> left;
    std::unique_ptr<LogicalExpression> right;
    
    LogicalOr(std::unique_ptr<LogicalExpression> l,
              std::unique_ptr<LogicalExpression> r)
        : left(std::move(l)), right(std::move(r)) {}
    
    std::string ToString(int indent = 0) const override {
        std::ostringstream oss;
        oss << Indent(indent) << "LogicalOr\n";
        oss << left->ToString(indent + 1) << "\n";
        oss << right->ToString(indent + 1);
        return oss.str();
    }
};

// ============================================================================
// WHERE условия
// ============================================================================

// Базовый класс для WHERE условий
class WhereClause : public ASTNode {
public:
    virtual ~WhereClause() = default;
};

// WHERE ALL - без фильтра
class WhereAll : public WhereClause {
public:
    std::string ToString(int indent = 0) const override {
        return Indent(indent) + "WhereAll";
    }
};

// WHERE ... LIKE ...
class WhereLike : public WhereClause {
public:
    std::unique_ptr<TextExpression> field;
    std::string pattern;
    bool notLike;
    
    WhereLike(std::unique_ptr<TextExpression> f, 
              const std::string& pat,
              bool neg = false)
        : field(std::move(f)), pattern(pat), notLike(neg) {}
    
    std::string ToString(int indent = 0) const override {
        std::ostringstream oss;
        oss << Indent(indent) << "WhereLike" << (notLike ? " NOT" : "") << "\n";
        oss << field->ToString(indent + 1) << "\n";
        oss << Indent(indent + 1) << "Pattern(\"" << pattern << "\")";
        return oss.str();
    }
};

// WHERE ... IN (...)
class WhereIn : public WhereClause {
public:
    std::unique_ptr<Expression> expr;
    std::vector<std::unique_ptr<Expression>> values;
    bool notIn;
    
    WhereIn(std::unique_ptr<Expression> e, bool neg = false)
        : expr(std::move(e)), notIn(neg) {}
    
    void addValue(std::unique_ptr<Expression> val) {
        values.push_back(std::move(val));
    }
    
    std::string ToString(int indent = 0) const override {
        std::ostringstream oss;
        oss << Indent(indent) << "WhereIn" << (notIn ? " NOT" : "") << "\n";
        oss << expr->ToString(indent + 1) << "\n";
        oss << Indent(indent + 1) << "Values:\n";
        for (const auto& v : values) {
            oss << v->ToString(indent + 2) << "\n";
        }
        return oss.str();
    }
};

// WHERE с логическим выражением
class WhereLogical : public WhereClause {
public:
    std::unique_ptr<LogicalExpression> condition;
    
    explicit WhereLogical(std::unique_ptr<LogicalExpression> cond)
        : condition(std::move(cond)) {}
    
    std::string ToString(int indent = 0) const override {
        std::ostringstream oss;
        oss << Indent(indent) << "WhereLogical\n";
        oss << condition->ToString(indent + 1);
        return oss.str();
    }
};

// ============================================================================
// SQL операторы (верхнеуровневые узлы)
// ============================================================================

// Базовый класс для всех SQL операторов
class SQLStatement : public ASTNode {
public:
    virtual ~SQLStatement() = default;
};

// CREATE TABLE
class CreateTableStatement : public SQLStatement {
public:
    struct FieldDef {
        std::string name;
        bool isText;  // true = TEXT, false = LONG
        unsigned textLen;  // длина для TEXT полей
        
        FieldDef() : isText(true), textLen(0) {}
        FieldDef(const std::string& n, bool text, unsigned len = 0)
            : name(n), isText(text), textLen(len) {}
    };
    
    std::string tableName;
    std::vector<FieldDef> fields;
    
    explicit CreateTableStatement(const std::string& name) : tableName(name) {}
    
    void addField(const FieldDef& f) {
        fields.push_back(f);
    }
    
    std::string ToString(int indent = 0) const override {
        std::ostringstream oss;
        oss << Indent(indent) << "CreateTableStatement: " << tableName << "\n";
        oss << Indent(indent + 1) << "Fields:\n";
        for (const auto& f : fields) {
            oss << Indent(indent + 2) << f.name << " ";
            if (f.isText) {
                oss << "TEXT(" << f.textLen << ")";
            } else {
                oss << "LONG";
            }
            oss << "\n";
        }
        return oss.str();
    }
};

// DROP TABLE
class DropTableStatement : public SQLStatement {
public:
    std::string tableName;
    
    explicit DropTableStatement(const std::string& name) : tableName(name) {}
    
    std::string ToString(int indent = 0) const override {
        return Indent(indent) + "DropTableStatement: " + tableName;
    }
};

// SELECT
class SelectStatement : public SQLStatement {
public:
    std::vector<std::string> fields;  // список полей, "*" означает все поля
    std::string tableName;
    std::unique_ptr<WhereClause> whereClause;
    bool selectAll;
    
    SelectStatement() : selectAll(false) {}
    
    void addField(const std::string& f) {
        fields.push_back(f);
    }
    
    std::string ToString(int indent = 0) const override {
        std::ostringstream oss;
        oss << Indent(indent) << "SelectStatement\n";
        oss << Indent(indent + 1) << "Fields: ";
        if (selectAll) {
            oss << "*";
        } else {
            for (size_t i = 0; i < fields.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << fields[i];
            }
        }
        oss << "\n";
        oss << Indent(indent + 1) << "Table: " << tableName << "\n";
        if (whereClause) {
            oss << whereClause->ToString(indent + 1) << "\n";
        } else {
            oss << Indent(indent + 1) << "WhereClause: none\n";
        }
        return oss.str();
    }
};

// INSERT INTO
class InsertStatement : public SQLStatement {
public:
    std::string tableName;
    std::vector<std::unique_ptr<Expression>> values;
    
    explicit InsertStatement(const std::string& name) : tableName(name) {}
    
    void addValue(std::unique_ptr<Expression> val) {
        values.push_back(std::move(val));
    }
    
    std::string ToString(int indent = 0) const override {
        std::ostringstream oss;
        oss << Indent(indent) << "InsertStatement: " << tableName << "\n";
        oss << Indent(indent + 1) << "Values:\n";
        for (const auto& v : values) {
            oss << v->ToString(indent + 2) << "\n";
        }
        return oss.str();
    }
};

// UPDATE
class UpdateStatement : public SQLStatement {
public:
    std::string tableName;
    std::string fieldName;
    std::unique_ptr<Expression> value;
    std::unique_ptr<WhereClause> whereClause;
    
    UpdateStatement() = default;
    
    std::string ToString(int indent = 0) const override {
        std::ostringstream oss;
        oss << Indent(indent) << "UpdateStatement: " << tableName << "\n";
        oss << Indent(indent + 1) << "Set: " << fieldName << " =\n";
        if (value) {
            oss << value->ToString(indent + 2) << "\n";
        }
        if (whereClause) {
            oss << whereClause->ToString(indent + 1) << "\n";
        } else {
            oss << Indent(indent + 1) << "WhereClause: none\n";
        }
        return oss.str();
    }
};

// DELETE FROM
class DeleteStatement : public SQLStatement {
public:
    std::string tableName;
    std::unique_ptr<WhereClause> whereClause;
    
    explicit DeleteStatement(const std::string& name) : tableName(name) {}
    
    std::string ToString(int indent = 0) const override {
        std::ostringstream oss;
        oss << Indent(indent) << "DeleteStatement: " << tableName << "\n";
        if (whereClause) {
            oss << whereClause->ToString(indent + 1) << "\n";
        } else {
            oss << Indent(indent + 1) << "WhereClause: none\n";
        }
        return oss.str();
    }
};

} // namespace sql

#endif // AST_H
