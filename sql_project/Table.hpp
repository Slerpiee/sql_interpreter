#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include "_Table.h"
#include <cstring>

// ─────────────────────────────────────────────────────────────
//  Exception hierarchy
// ─────────────────────────────────────────────────────────────
class TableException : public std::runtime_error {
public:
    explicit TableException(const std::string& msg) : std::runtime_error(msg) {}
    explicit TableException(enum Errors code)
        : std::runtime_error(getErrorString(code)), errCode(code) {}
    enum Errors errCode = OK;
};

class TableNotFoundException  : public TableException {
public: using TableException::TableException;
};
class TableFieldException     : public TableException {
public: using TableException::TableException;
};
class TableIOException        : public TableException {
public: using TableException::TableException;
};

// ─────────────────────────────────────────────────────────────
//  Field description (for CREATE TABLE)
// ─────────────────────────────────────────────────────────────
struct ColumnDef {
    std::string name;
    enum FieldType type;
    long textLen = 0; // only for Text fields

    static ColumnDef longCol(const std::string& n)                { return {n, Long, 0}; }
    static ColumnDef textCol(const std::string& n, long len)      { return {n, Text, len}; }
};

// ─────────────────────────────────────────────────────────────
//  Value variant (Long or Text)
// ─────────────────────────────────────────────────────────────
struct Value {
    enum FieldType type;
    long   longVal = 0;
    std::string textVal;

    static Value fromLong(long v)              { Value x; x.type=Long; x.longVal=v; return x; }
    static Value fromText(const std::string& s){ Value x; x.type=Text; x.textVal=s; return x; }

    std::string toString() const {
        return type==Long ? std::to_string(longVal) : textVal;
    }
};

// ─────────────────────────────────────────────────────────────
//  Row: a map-like ordered container of (name → value)
// ─────────────────────────────────────────────────────────────
struct Row {
    std::vector<std::string> columns;
    std::vector<Value>       values;

    void set(const std::string& col, Value v) {
        for (size_t i = 0; i < columns.size(); ++i)
            if (columns[i] == col) { values[i] = v; return; }
        columns.push_back(col);
        values.push_back(v);
    }

    const Value* get(const std::string& col) const {
        for (size_t i = 0; i < columns.size(); ++i)
            if (columns[i] == col) return &values[i];
        return nullptr;
    }
};

// ─────────────────────────────────────────────────────────────
//  ResultSet returned to the client
// ─────────────────────────────────────────────────────────────
struct ResultSet {
    std::vector<std::string> columns;
    std::vector<Row>         rows;
    std::string              message; // for non-SELECT statements
    int                      affected = 0;
};

// ─────────────────────────────────────────────────────────────
//  Table – RAII wrapper around THandle
// ─────────────────────────────────────────────────────────────
class Table {
public:
    // ── static factory / lifecycle ──────────────────────────
    static void create(const std::string& name,
                       const std::vector<ColumnDef>& cols) {
        std::vector<FieldDef> fds(cols.size());
        for (size_t i = 0; i < cols.size(); ++i) {
            strncpy(fds[i].fieldName, cols[i].name.c_str(), MaxFieldNameLen-1);
            fds[i].type = cols[i].type;
            fds[i].len  = cols[i].textLen;
        }
        TableStruct ts;
        ts.numOfFields = (unsigned)cols.size();
        ts.fieldsDef   = fds.data();
        checkOK(::createTable(const_cast<char*>(name.c_str()), &ts));
    }

    static void drop(const std::string& name) {
        checkOK(::deleteTable(const_cast<char*>(name.c_str())));
    }

    explicit Table(const std::string& name) : name_(name) {
        enum Errors e = openTable(const_cast<char*>(name.c_str()), &handle_);
        if (e != OK) throw TableNotFoundException(e);
    }

    ~Table() {
        if (handle_) closeTable(handle_);
    }

    // Non-copyable, movable
    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;

    // ── column introspection ────────────────────────────────
    std::vector<std::string> columnNames() const {
        unsigned n = 0;
        getFieldsNum(handle_, &n);
        std::vector<std::string> res;
        for (unsigned i = 0; i < n; ++i) {
            char* nm = nullptr;
            getFieldName(handle_, i, &nm);
            res.push_back(nm ? nm : "");
        }
        return res;
    }

    enum FieldType columnType(const std::string& col) const {
        enum FieldType t = Long;
        checkOK(::getFieldType(handle_, const_cast<char*>(col.c_str()), &t));
        return t;
    }

    // ── cursor navigation ───────────────────────────────────
    bool first()    { return moveFirst(handle_)    == OK && !isBeforeFirst(); }
    bool last()     { return moveLast(handle_)     == OK && !isAfterLast(); }
    bool next()     { return moveNext(handle_)     == OK && !isAfterLast(); }
    bool isAfterLast()   const { return afterLast(handle_)   == TRUE; }
    bool isBeforeFirst() const { return beforeFirst(handle_) == TRUE; }

    // ── read current row ─────────────────────────────────────
    Row currentRow() const {
        Row row;
        auto cols = columnNames();
        for (auto& c : cols) {
            enum FieldType t = Long;
            ::getFieldType(handle_, const_cast<char*>(c.c_str()), &t);
            if (t == Long) {
                long v = 0;
                ::getLong(handle_, const_cast<char*>(c.c_str()), &v);
                row.set(c, Value::fromLong(v));
            } else {
                char* p = nullptr;
                ::getText(handle_, const_cast<char*>(c.c_str()), &p);
                row.set(c, Value::fromText(p ? p : ""));
            }
        }
        return row;
    }

    // ── insert a new row ─────────────────────────────────────
    void insert(const Row& row) {
        checkOK(::createNew(handle_));
        for (size_t i = 0; i < row.columns.size(); ++i) {
            const std::string& c = row.columns[i];
            const Value& v       = row.values[i];
            if (v.type == Long) {
                checkOK(::putLongNew(handle_, const_cast<char*>(c.c_str()), v.longVal));
            } else {
                checkOK(::putTextNew(handle_, const_cast<char*>(c.c_str()),
                                     const_cast<char*>(v.textVal.c_str())));
            }
        }
        checkOK(::insertzNew(handle_));
    }

    // ── update current row (caller must iterate) ─────────────
    void updateCurrent(const std::vector<std::string>& cols,
                       const std::vector<Value>& vals) {
        checkOK(::startEdit(handle_));
        for (size_t i = 0; i < cols.size(); ++i) {
            if (vals[i].type == Long)
                checkOK(::putLong(handle_, const_cast<char*>(cols[i].c_str()), vals[i].longVal));
            else
                checkOK(::putText(handle_, const_cast<char*>(cols[i].c_str()),
                                  const_cast<char*>(vals[i].textVal.c_str())));
        }
        checkOK(::finishEdit(handle_));
    }

    // ── delete current row ───────────────────────────────────
    void deleteCurrent() {
        checkOK(::deleteRec(handle_));
    }

    const std::string& tableName() const { return name_; }

private:
    THandle     handle_ = nullptr;
    std::string name_;

    static void checkOK(enum Errors e) {
        if (e == OK) return;
        switch (e) {
            case CantOpenTable:
            case BadFileName:
            case CantCreateTable:
                throw TableNotFoundException(e);
            case FieldNotFound:
            case BadFieldType:
            case BadFieldLen:
                throw TableFieldException(e);
            default:
                throw TableIOException(e);
        }
    }
};
