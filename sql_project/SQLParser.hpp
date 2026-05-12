#pragma once
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <memory>

// ─────────────────────────────────────────────────────────────
//  Parse exception
// ─────────────────────────────────────────────────────────────
class ParseException : public std::runtime_error {
public:
    explicit ParseException(const std::string& msg) : std::runtime_error("Parse error: " + msg) {}
};

// ─────────────────────────────────────────────────────────────
//  Tokenizer
// ─────────────────────────────────────────────────────────────
enum class TokType {
    IDENT, NUMBER, STRING,
    COMMA, LPAREN, RPAREN, STAR, SEMICOLON, EQ, NEQ, LT, GT, LE, GE,
    AND, OR, NOT,
    END
};

struct Token {
    TokType     type;
    std::string value;
};

class Tokenizer {
public:
    explicit Tokenizer(const std::string& src) : src_(src), pos_(0) {}

    std::vector<Token> tokenize() {
        std::vector<Token> toks;
        while (pos_ < src_.size()) {
            skipWS();
            if (pos_ >= src_.size()) break;
            char c = src_[pos_];

            if (c == '\'') { toks.push_back(readString()); continue; }
            if (std::isdigit(c) || (c=='-' && pos_+1<src_.size() && std::isdigit(src_[pos_+1]))) {
                toks.push_back(readNumber()); continue;
            }
            if (std::isalpha(c) || c=='_') { toks.push_back(readIdent()); continue; }
            switch(c) {
                case ',': toks.push_back({TokType::COMMA,   ","}); pos_++; break;
                case '(': toks.push_back({TokType::LPAREN,  "("}); pos_++; break;
                case ')': toks.push_back({TokType::RPAREN,  ")"}); pos_++; break;
                case '*': toks.push_back({TokType::STAR,    "*"}); pos_++; break;
                case ';': toks.push_back({TokType::SEMICOLON,";"}); pos_++; break;
                case '=': toks.push_back({TokType::EQ,      "="}); pos_++; break;
                case '<':
                    if (pos_+1<src_.size() && src_[pos_+1]=='=')
                        { toks.push_back({TokType::LE,"<="}); pos_+=2; }
                    else if (pos_+1<src_.size() && src_[pos_+1]=='>')
                        { toks.push_back({TokType::NEQ,"<>"}); pos_+=2; }
                    else
                        { toks.push_back({TokType::LT,"<"}); pos_++; }
                    break;
                case '>':
                    if (pos_+1<src_.size() && src_[pos_+1]=='=')
                        { toks.push_back({TokType::GE,">="}); pos_+=2; }
                    else
                        { toks.push_back({TokType::GT,">"}); pos_++; }
                    break;
                default:
                    throw ParseException(std::string("Unexpected character: ")+c);
            }
        }
        toks.push_back({TokType::END,""});
        return toks;
    }

private:
    std::string src_;
    size_t pos_;

    void skipWS() { while (pos_<src_.size() && std::isspace(src_[pos_])) pos_++; }

    Token readString() {
        pos_++; // skip opening '
        std::string val;
        while (pos_<src_.size() && src_[pos_]!='\'') val+=src_[pos_++];
        if (pos_>=src_.size()) throw ParseException("Unterminated string literal");
        pos_++; // skip closing '
        return {TokType::STRING, val};
    }

    Token readNumber() {
        std::string val;
        if (src_[pos_]=='-') val+=src_[pos_++];
        while (pos_<src_.size() && std::isdigit(src_[pos_])) val+=src_[pos_++];
        return {TokType::NUMBER, val};
    }

    Token readIdent() {
        std::string val;
        while (pos_<src_.size() && (std::isalnum(src_[pos_])||src_[pos_]=='_')) val+=src_[pos_++];
        // classify keywords
        std::string upper = val;
        std::transform(upper.begin(),upper.end(),upper.begin(),::toupper);
        if (upper=="AND") return {TokType::AND, upper};
        if (upper=="OR")  return {TokType::OR,  upper};
        if (upper=="NOT") return {TokType::NOT, upper};
        return {TokType::IDENT, val};
    }
};

// ─────────────────────────────────────────────────────────────
//  AST nodes
// ─────────────────────────────────────────────────────────────
using LiteralValue = std::variant<long, std::string>;

struct CondExpr; // forward

// A single comparison: col OP literal
struct Comparison {
    std::string column;
    std::string op; // =, <>, <, >, <=, >=
    LiteralValue value;
};

// Condition tree
struct CondExpr {
    enum class Kind { Cmp, And, Or, Not } kind;
    std::optional<Comparison> cmp;
    std::shared_ptr<CondExpr> left, right; // for AND/OR
    std::shared_ptr<CondExpr> child;       // for NOT
};

// Column definition for CREATE TABLE
struct ColDefAST {
    std::string name;
    std::string type;   // "LONG" or "TEXT"
    int         len = 0;
};

// Assignment for UPDATE SET
struct Assignment {
    std::string column;
    LiteralValue value;
};

// SELECT columns
struct SelectCols {
    bool star = false;
    std::vector<std::string> names;
};

// ─────────────────────────────────────────────────────────────
//  Statement variants
// ─────────────────────────────────────────────────────────────
struct SelectStmt {
    SelectCols cols;
    std::string table;
    std::shared_ptr<CondExpr> where; // nullptr = no WHERE
};

struct InsertStmt {
    std::string table;
    std::vector<std::string> columns;
    std::vector<LiteralValue> values;
};

struct UpdateStmt {
    std::string table;
    std::vector<Assignment> assignments;
    std::shared_ptr<CondExpr> where;
};

struct DeleteStmt {
    std::string table;
    std::shared_ptr<CondExpr> where;
};

struct CreateTableStmt {
    std::string table;
    std::vector<ColDefAST> cols;
};

struct DropTableStmt {
    std::string table;
};

using Statement = std::variant<
    SelectStmt, InsertStmt, UpdateStmt, DeleteStmt,
    CreateTableStmt, DropTableStmt
>;

// ─────────────────────────────────────────────────────────────
//  Parser
// ─────────────────────────────────────────────────────────────
class SQLParser {
public:
    explicit SQLParser(const std::string& sql) {
        Tokenizer tok(sql);
        tokens_ = tok.tokenize();
        pos_ = 0;
    }

    Statement parse() {
        auto stmt = parseStatement();
        // optional semicolon
        if (cur().type == TokType::SEMICOLON) advance();
        if (cur().type != TokType::END)
            throw ParseException("Unexpected token after statement: '" + cur().value + "'");
        return stmt;
    }

private:
    std::vector<Token> tokens_;
    size_t pos_;

    const Token& cur()  const { return tokens_[pos_]; }
    Token advance()           { return tokens_[pos_++]; }

    bool check(TokType t) const { return cur().type == t; }

    Token expect(TokType t, const std::string& what) {
        if (cur().type != t) throw ParseException("Expected " + what + ", got '" + cur().value + "'");
        return advance();
    }

    // case-insensitive ident check
    bool isKeyword(const std::string& kw) const {
        if (cur().type != TokType::IDENT) return false;
        std::string u = cur().value;
        std::transform(u.begin(),u.end(),u.begin(),::toupper);
        return u == kw;
    }
    void expectKW(const std::string& kw) {
        if (!isKeyword(kw)) throw ParseException("Expected keyword '" + kw + "', got '" + cur().value + "'");
        advance();
    }
    std::string upperVal() const {
        std::string u = cur().value;
        std::transform(u.begin(),u.end(),u.begin(),::toupper);
        return u;
    }

    // ── statement dispatcher ─────────────────────────────────
    Statement parseStatement() {
        if (cur().type != TokType::IDENT)
            throw ParseException("Expected SQL keyword, got '" + cur().value + "'");
        std::string kw = upperVal();
        if      (kw=="SELECT") return parseSelect();
        else if (kw=="INSERT") return parseInsert();
        else if (kw=="UPDATE") return parseUpdate();
        else if (kw=="DELETE") return parseDelete();
        else if (kw=="CREATE") return parseCreate();
        else if (kw=="DROP")   return parseDrop();
        throw ParseException("Unknown statement: '" + cur().value + "'");
    }

    // ── SELECT ───────────────────────────────────────────────
    SelectStmt parseSelect() {
        expectKW("SELECT");
        SelectCols cols;
        if (check(TokType::STAR)) { cols.star=true; advance(); }
        else {
            cols.names.push_back(expect(TokType::IDENT,"column name").value);
            while (check(TokType::COMMA)) { advance(); cols.names.push_back(expect(TokType::IDENT,"column name").value); }
        }
        expectKW("FROM");
        std::string tbl = expect(TokType::IDENT,"table name").value;
        std::shared_ptr<CondExpr> where;
        if (isKeyword("WHERE")) { advance(); where = parseCondExpr(); }
        return {cols, tbl, where};
    }

    // ── INSERT ───────────────────────────────────────────────
    InsertStmt parseInsert() {
        expectKW("INSERT"); expectKW("INTO");
        std::string tbl = expect(TokType::IDENT,"table name").value;
        expect(TokType::LPAREN,"(");
        std::vector<std::string> cols;
        cols.push_back(expect(TokType::IDENT,"column").value);
        while (check(TokType::COMMA)) { advance(); cols.push_back(expect(TokType::IDENT,"column").value); }
        expect(TokType::RPAREN,")");
        expectKW("VALUES");
        expect(TokType::LPAREN,"(");
        std::vector<LiteralValue> vals;
        vals.push_back(parseLiteral());
        while (check(TokType::COMMA)) { advance(); vals.push_back(parseLiteral()); }
        expect(TokType::RPAREN,")");
        if (cols.size()!=vals.size()) throw ParseException("Column/value count mismatch");
        return {tbl, cols, vals};
    }

    // ── UPDATE ───────────────────────────────────────────────
    UpdateStmt parseUpdate() {
        expectKW("UPDATE");
        std::string tbl = expect(TokType::IDENT,"table name").value;
        expectKW("SET");
        std::vector<Assignment> asgns;
        asgns.push_back(parseAssignment());
        while (check(TokType::COMMA)) { advance(); asgns.push_back(parseAssignment()); }
        std::shared_ptr<CondExpr> where;
        if (isKeyword("WHERE")) { advance(); where = parseCondExpr(); }
        return {tbl, asgns, where};
    }

    Assignment parseAssignment() {
        std::string col = expect(TokType::IDENT,"column").value;
        expect(TokType::EQ,"=");
        return {col, parseLiteral()};
    }

    // ── DELETE ───────────────────────────────────────────────
    DeleteStmt parseDelete() {
        expectKW("DELETE"); expectKW("FROM");
        std::string tbl = expect(TokType::IDENT,"table name").value;
        std::shared_ptr<CondExpr> where;
        if (isKeyword("WHERE")) { advance(); where = parseCondExpr(); }
        return {tbl, where};
    }

    // ── CREATE TABLE ─────────────────────────────────────────
    CreateTableStmt parseCreate() {
        expectKW("CREATE"); expectKW("TABLE");
        std::string tbl = expect(TokType::IDENT,"table name").value;
        expect(TokType::LPAREN,"(");
        std::vector<ColDefAST> cols;
        cols.push_back(parseColDef());
        while (check(TokType::COMMA)) { advance(); cols.push_back(parseColDef()); }
        expect(TokType::RPAREN,")");
        return {tbl, cols};
    }

    ColDefAST parseColDef() {
        std::string name = expect(TokType::IDENT,"column name").value;
        std::string type = upperVal();
        if (type!="LONG" && type!="TEXT")
            throw ParseException("Expected LONG or TEXT, got '" + cur().value + "'");
        advance();
        int len = 0;
        if (type=="TEXT") {
            expect(TokType::LPAREN,"(");
            len = std::stoi(expect(TokType::NUMBER,"length").value);
            expect(TokType::RPAREN,")");
        }
        return {name, type, len};
    }

    // ── DROP TABLE ───────────────────────────────────────────
    DropTableStmt parseDrop() {
        expectKW("DROP"); expectKW("TABLE");
        return {expect(TokType::IDENT,"table name").value};
    }

    // ── Condition expression (AND/OR/NOT/comparison) ─────────
    std::shared_ptr<CondExpr> parseCondExpr() { return parseOr(); }

    std::shared_ptr<CondExpr> parseOr() {
        auto left = parseAnd();
        while (cur().type == TokType::OR) {
            advance();
            auto right = parseAnd();
            auto node = std::make_shared<CondExpr>();
            node->kind  = CondExpr::Kind::Or;
            node->left  = left;
            node->right = right;
            left = node;
        }
        return left;
    }

    std::shared_ptr<CondExpr> parseAnd() {
        auto left = parseNot();
        while (cur().type == TokType::AND) {
            advance();
            auto right = parseNot();
            auto node = std::make_shared<CondExpr>();
            node->kind  = CondExpr::Kind::And;
            node->left  = left;
            node->right = right;
            left = node;
        }
        return left;
    }

    std::shared_ptr<CondExpr> parseNot() {
        if (cur().type == TokType::NOT) {
            advance();
            auto node = std::make_shared<CondExpr>();
            node->kind  = CondExpr::Kind::Not;
            node->child = parseNot();
            return node;
        }
        return parseCmp();
    }

    std::shared_ptr<CondExpr> parseCmp() {
        // handle sub-expression in parens
        if (check(TokType::LPAREN)) {
            advance();
            auto e = parseCondExpr();
            expect(TokType::RPAREN,")");
            return e;
        }
        std::string col = expect(TokType::IDENT,"column").value;
        std::string op  = parseOp();
        LiteralValue val = parseLiteral();
        auto node = std::make_shared<CondExpr>();
        node->kind = CondExpr::Kind::Cmp;
        node->cmp  = Comparison{col, op, val};
        return node;
    }

    std::string parseOp() {
        switch(cur().type) {
            case TokType::EQ:  advance(); return "=";
            case TokType::NEQ: advance(); return "<>";
            case TokType::LT:  advance(); return "<";
            case TokType::GT:  advance(); return ">";
            case TokType::LE:  advance(); return "<=";
            case TokType::GE:  advance(); return ">=";
            default: throw ParseException("Expected comparison operator, got '" + cur().value + "'");
        }
    }

    LiteralValue parseLiteral() {
        if (check(TokType::NUMBER)) return (long)std::stol(advance().value);
        if (check(TokType::STRING)) return advance().value;
        if (check(TokType::IDENT)) {
            // allow bare identifiers as string values (e.g. status = active)
            return advance().value;
        }
        throw ParseException("Expected literal value, got '" + cur().value + "'");
    }
};
