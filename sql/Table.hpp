#ifndef TABLE_HPP
#define TABLE_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
#include "TableTypes.h"

namespace table {

/**
 * @brief Exception class for Table errors
 */
class TableException : public std::runtime_error {
public:
    explicit TableException(const std::string& message) 
        : std::runtime_error(message) {}
    
    TableException(enum Errors error, const std::string& context = "")
        : std::runtime_error(context + ": " + getErrorString(error)) {}
};

/**
 * @brief Field definition for table schema
 */
struct FieldDefinition {
    std::string name;
    enum FieldType type;
    unsigned length;  // Only used for Text fields
    
    FieldDefinition(const std::string& n, enum FieldType t, unsigned l = 0)
        : name(n), type(t), length(l) {}
};

/**
 * @brief RAII wrapper for Table operations
 * 
 * This class provides a C++ interface to the C-based Table library.
 * It handles resource management automatically and provides exception-safe operations.
 */
class Table {
public:
    /**
     * @brief Create a new table file with the specified schema
     * @param tableName Name of the table file
     * @param fields Vector of field definitions
     * @throws TableException if table creation fails
     */
    static void create(const std::string& tableName, const std::vector<FieldDefinition>& fields) {
        std::vector<struct FieldDef> cFields(fields.size());
        
        for (size_t i = 0; i < fields.size(); ++i) {
            if (fields[i].name.length() >= MaxFieldNameLen) {
                throw TableException(BadArgs, "Field name too long");
            }
            std::strncpy(cFields[i].name, fields[i].name.c_str(), MaxFieldNameLen - 1);
            cFields[i].name[MaxFieldNameLen - 1] = '\0';
            cFields[i].type = fields[i].type;
            cFields[i].len = fields[i].length;
        }
        
        struct TableStruct tableStruct;
        tableStruct.numOfFields = static_cast<long>(fields.size());
        tableStruct.fieldsDef = cFields.data();
        
        enum Errors err = ::createTable(const_cast<char*>(tableName.c_str()), &tableStruct);
        if (err != OK) {
            throw TableException(err, "Failed to create table: " + tableName);
        }
    }
    
    /**
     * @brief Delete a table file
     * @param tableName Name of the table file
     * @throws TableException if deletion fails
     */
    static void remove(const std::string& tableName) {
        enum Errors err = ::deleteTable(const_cast<char*>(tableName.c_str()));
        if (err != OK) {
            throw TableException(err, "Failed to delete table: " + tableName);
        }
    }
    
    /**
     * @brief Open an existing table
     * @param tableName Name of the table file
     * @throws TableException if opening fails
     */
    explicit Table(const std::string& tableName) : handle_(nullptr), tableName_(tableName) {
        enum Errors err = ::openTable(const_cast<char*>(tableName.c_str()), &handle_);
        if (err != OK || !handle_) {
            throw TableException(err, "Failed to open table: " + tableName);
        }
    }
    
    /**
     * @brief Destructor - closes the table
     */
    ~Table() {
        if (handle_) {
            ::closeTable(handle_);
        }
    }
    
    // Disable copy
    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;
    
    // Enable move
    Table(Table&& other) noexcept : handle_(other.handle_), tableName_(std::move(other.tableName_)) {
        other.handle_ = nullptr;
    }
    
    Table& operator=(Table&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                ::closeTable(handle_);
            }
            handle_ = other.handle_;
            tableName_ = std::move(other.tableName_);
            other.handle_ = nullptr;
        }
        return *this;
    }
    
    /**
     * @brief Move cursor to the first record
     * @throws TableException on error
     */
    void moveFirst() {
        enum Errors err = ::moveFirst(handle_);
        if (err != OK) {
            throw TableException(err, "moveFirst failed");
        }
    }
    
    /**
     * @brief Move cursor to the last record
     * @throws TableException on error
     */
    void moveLast() {
        enum Errors err = ::moveLast(handle_);
        if (err != OK) {
            throw TableException(err, "moveLast failed");
        }
    }
    
    /**
     * @brief Move cursor to the next record
     * @throws TableException on error
     */
    void moveNext() {
        enum Errors err = ::moveNext(handle_);
        if (err != OK) {
            throw TableException(err, "moveNext failed");
        }
    }
    
    /**
     * @brief Move cursor to the previous record
     * @throws TableException on error
     */
    void movePrevious() {
        enum Errors err = ::movePrevios(handle_);
        if (err != OK) {
            throw TableException(err, "movePrevious failed");
        }
    }
    
    /**
     * @brief Check if cursor is before the first record
     * @return true if before first record
     */
    bool isBeforeFirst() const {
        return ::beforeFirst(const_cast<THandle>(handle_));
    }
    
    /**
     * @brief Check if cursor is after the last record
     * @return true if after last record
     */
    bool isAfterLast() const {
        return ::afterLast(const_cast<THandle>(handle_));
    }
    
    /**
     * @brief Get text field value from current record
     * @param fieldName Name of the field
     * @return Field value as string
     * @throws TableException on error
     */
    std::string getText(const std::string& fieldName) const {
        char* value;
        enum Errors err = ::getText(const_cast<THandle>(handle_), 
                                     const_cast<char*>(fieldName.c_str()), &value);
        if (err != OK) {
            throw TableException(err, "Failed to get text field: " + fieldName);
        }
        return std::string(value);
    }
    
    /**
     * @brief Get long field value from current record
     * @param fieldName Name of the field
     * @return Field value as long
     * @throws TableException on error
     */
    long getLong(const std::string& fieldName) const {
        long value;
        enum Errors err = ::getLong(const_cast<THandle>(handle_),
                                     const_cast<char*>(fieldName.c_str()), &value);
        if (err != OK) {
            throw TableException(err, "Failed to get long field: " + fieldName);
        }
        return value;
    }
    
    /**
     * @brief Start editing the current record
     * @throws TableException on error
     */
    void startEdit() {
        enum Errors err = ::startEdit(handle_);
        if (err != OK) {
            throw TableException(err, "Failed to start edit");
        }
    }
    
    /**
     * @brief Set text field value during edit
     * @param fieldName Name of the field
     * @param value New value
     * @throws TableException on error
     */
    void putText(const std::string& fieldName, const std::string& value) {
        enum Errors err = ::putText(handle_, 
                                     const_cast<char*>(fieldName.c_str()),
                                     const_cast<char*>(value.c_str()));
        if (err != OK) {
            throw TableException(err, "Failed to set text field: " + fieldName);
        }
    }
    
    /**
     * @brief Set long field value during edit
     * @param fieldName Name of the field
     * @param value New value
     * @throws TableException on error
     */
    void putLong(const std::string& fieldName, long value) {
        enum Errors err = ::putLong(handle_,
                                     const_cast<char*>(fieldName.c_str()), value);
        if (err != OK) {
            throw TableException(err, "Failed to set long field: " + fieldName);
        }
    }
    
    /**
     * @brief Finish editing and save changes
     * @throws TableException on error
     */
    void finishEdit() {
        enum Errors err = ::finishEdit(handle_);
        if (err != OK) {
            throw TableException(err, "Failed to finish edit");
        }
    }
    
    /**
     * @brief Prepare for inserting a new record
     * @throws TableException on error
     */
    void createNew() {
        enum Errors err = ::createNew(handle_);
        if (err != OK) {
            throw TableException(err, "Failed to create new record");
        }
    }
    
    /**
     * @brief Set text field value for new record
     * @param fieldName Name of the field
     * @param value Value to set
     * @throws TableException on error
     */
    void putTextNew(const std::string& fieldName, const std::string& value) {
        enum Errors err = ::putTextNew(handle_,
                                        const_cast<char*>(fieldName.c_str()),
                                        const_cast<char*>(value.c_str()));
        if (err != OK) {
            throw TableException(err, "Failed to set text field for new record: " + fieldName);
        }
    }
    
    /**
     * @brief Set long field value for new record
     * @param fieldName Name of the field
     * @param value Value to set
     * @throws TableException on error
     */
    void putLongNew(const std::string& fieldName, long value) {
        enum Errors err = ::putLongNew(handle_,
                                        const_cast<char*>(fieldName.c_str()), value);
        if (err != OK) {
            throw TableException(err, "Failed to set long field for new record: " + fieldName);
        }
    }
    
    /**
     * @brief Insert new record at current position
     * @throws TableException on error
     */
    void insertNew() {
        enum Errors err = ::insertNew(handle_);
        if (err != OK) {
            throw TableException(err, "Failed to insert new record");
        }
    }
    
    /**
     * @brief Insert new record at the beginning
     * @throws TableException on error
     */
    void insertAtBeginning() {
        enum Errors err = ::insertaNew(handle_);
        if (err != OK) {
            throw TableException(err, "Failed to insert record at beginning");
        }
    }
    
    /**
     * @brief Insert new record at the end
     * @throws TableException on error
     */
    void insertAtEnd() {
        enum Errors err = ::insertzNew(handle_);
        if (err != OK) {
            throw TableException(err, "Failed to insert record at end");
        }
    }
    
    /**
     * @brief Delete the current record
     * @throws TableException on error
     */
    void deleteRecord() {
        enum Errors err = ::deleteRec(handle_);
        if (err != OK) {
            throw TableException(err, "Failed to delete record");
        }
    }
    
    /**
     * @brief Get the number of fields in the table
     * @return Number of fields
     * @throws TableException on error
     */
    unsigned getFieldCount() const {
        unsigned num;
        enum Errors err = ::getFieldsNum(const_cast<THandle>(handle_), &num);
        if (err != OK) {
            throw TableException(err, "Failed to get field count");
        }
        return num;
    }
    
    /**
     * @brief Get field name by index
     * @param index Field index
     * @return Field name
     * @throws TableException on error
     */
    std::string getFieldName(unsigned index) const {
        char* name;
        enum Errors err = ::getFieldName(const_cast<THandle>(handle_), index, &name);
        if (err != OK) {
            throw TableException(err, "Failed to get field name");
        }
        return std::string(name);
    }
    
    /**
     * @brief Get field type by name
     * @param fieldName Name of the field
     * @return Field type
     * @throws TableException on error
     */
    enum FieldType getFieldType(const std::string& fieldName) const {
        enum FieldType type;
        enum Errors err = ::getFieldType(const_cast<THandle>(handle_),
                                          const_cast<char*>(fieldName.c_str()), &type);
        if (err != OK) {
            throw TableException(err, "Failed to get field type: " + fieldName);
        }
        return type;
    }
    
    /**
     * @brief Get field length by name
     * @param fieldName Name of the field
     * @return Field length
     * @throws TableException on error
     */
    unsigned getFieldLength(const std::string& fieldName) const {
        unsigned len;
        enum Errors err = ::getFieldLen(const_cast<THandle>(handle_),
                                         const_cast<char*>(fieldName.c_str()), &len);
        if (err != OK) {
            throw TableException(err, "Failed to get field length: " + fieldName);
        }
        return len;
    }
    
    /**
     * @brief Get the underlying C handle (for advanced usage)
     * @return C table handle
     */
    THandle getHandle() const { return handle_; }
    
    /**
     * @brief Get the table name
     * @return Table name
     */
    const std::string& getTableName() const { return tableName_; }

private:
    THandle handle_;
    std::string tableName_;
};

} // namespace table

#endif // TABLE_HPP
