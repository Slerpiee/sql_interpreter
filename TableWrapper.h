#ifndef TABLE_WRAPPER_H
#define TABLE_WRAPPER_H

#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
#include "_Table.h"

namespace TableLib {

// Исключение для ошибок работы с таблицей
class TableException : public std::runtime_error {
public:
    explicit TableException(const std::string& message) 
        : std::runtime_error(message) {}
};

// Перечисление типов полей
enum class FieldType {
    Long = 0,
    Text = 1
};

// Перечисление кодов ошибок
enum class ErrorCode {
    OK = 0,
    CantCreateTable = 1,
    CantOpenTable = 2,
    FieldNotFound = 3,
    BadHandle = 4,
    BadArgs = 5,
    CantMoveToPos = 6,
    FileWriteError = 7,
    FileReadError = 8,
    CorruptedData = 9,
    CantCreateHandle = 10,
    ReadOnlyFile = 11,
    BadFileName = 12,
    CantDeleteTable = 13,
    BadFilePosition = 14,
    BadFieldType = 15,
    BadFieldLen = 16,
    NoEditing = 17,
    BadPos = 18
};

// Структура для определения поля
struct FieldDefinition {
    std::string name;
    FieldType type;
    unsigned long length;
    
    FieldDefinition(const std::string& n, FieldType t, unsigned long l)
        : name(n), type(t), length(l) {}
};

// Класс-обертка для таблицы
class Table {
public:
    // Конструктор по умолчанию (пустая таблица)
    Table() : handle_(nullptr) {}
    
    // Деструктор
    ~Table() {
        close();
    }
    
    // Запрет копирования
    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;
    
    // Разрешение перемещения
    Table(Table&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    
    Table& operator=(Table&& other) noexcept {
        if (this != &other) {
            close();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    
    // Создание новой таблицы
    static void create(const std::string& tableName, const std::vector<FieldDefinition>& fields) {
        if (fields.empty()) {
            throw TableException("Fields list cannot be empty");
        }
        
        // Создаем массив структур ::FieldDefinition
        std::vector<::FieldDefinition> cFields(fields.size());
        for (size_t i = 0; i < fields.size(); ++i) {
            strncpy(cFields[i].fieldName, fields[i].name.c_str(), MaxFieldNameLen - 1);
            cFields[i].fieldName[MaxFieldNameLen - 1] = '\0';
            cFields[i].type = static_cast<::FieldType>(fields[i].type);
            cFields[i].len = fields[i].length;
        }
        
        ::TableStruct tableStruct;
        tableStruct.numOfFields = static_cast<long>(fields.size());
        tableStruct.fieldsDef = cFields.data();
        
        ::Errors err = ::createTable(const_cast<char*>(tableName.c_str()), &tableStruct);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    // Удаление таблицы
    static void drop(const std::string& tableName) {
        ::Errors err = ::deleteTable(const_cast<char*>(tableName.c_str()));
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    // Открытие существующей таблицы
    void open(const std::string& tableName) {
        if (handle_ != nullptr) {
            throw TableException("Table is already open");
        }
        
        ::Errors err = ::openTable(const_cast<char*>(tableName.c_str()), &handle_);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    // Закрытие таблицы
    void close() {
        if (handle_ != nullptr) {
            ::closeTable(handle_);
            handle_ = nullptr;
        }
    }
    
    // Навигация по записям
    void moveFirst() {
        checkHandle();
        ::Errors err = ::moveFirst(handle_);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    void moveLast() {
        checkHandle();
        ::Errors err = ::moveLast(handle_);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    void moveNext() {
        checkHandle();
        ::Errors err = ::moveNext(handle_);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    void movePrevious() {
        checkHandle();
        ::Errors err = ::movePrevios(handle_);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    bool isBeforeFirst() const {
        if (!handle_) return false;
        return ::beforeFirst(handle_);
    }
    
    bool isAfterLast() const {
        if (!handle_) return false;
        return ::afterLast(handle_);
    }
    
    // Чтение значений полей текущей записи
    std::string getText(const std::string& fieldName) {
        checkHandle();
        char* value = nullptr;
        ::Errors err = ::getText(handle_, const_cast<char*>(fieldName.c_str()), &value);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
        return std::string(value);
    }
    
    long getLong(const std::string& fieldName) {
        checkHandle();
        long value = 0;
        ::Errors err = ::getLong(handle_, const_cast<char*>(fieldName.c_str()), &value);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
        return value;
    }
    
    // Редактирование текущей записи
    void startEdit() {
        checkHandle();
        ::Errors err = ::startEdit(handle_);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    void putText(const std::string& fieldName, const std::string& value) {
        checkHandle();
        ::Errors err = ::putText(handle_, const_cast<char*>(fieldName.c_str()), 
                                  const_cast<char*>(value.c_str()));
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    void putLong(const std::string& fieldName, long value) {
        checkHandle();
        ::Errors err = ::putLong(handle_, const_cast<char*>(fieldName.c_str()), value);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    void finishEdit() {
        checkHandle();
        ::Errors err = ::finishEdit(handle_);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    // Вставка новой записи
    void createNew() {
        checkHandle();
        ::Errors err = ::createNew(handle_);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    void putTextNew(const std::string& fieldName, const std::string& value) {
        checkHandle();
        ::Errors err = ::putTextNew(handle_, const_cast<char*>(fieldName.c_str()),
                                     const_cast<char*>(value.c_str()));
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    void putLongNew(const std::string& fieldName, long value) {
        checkHandle();
        ::Errors err = ::putLongNew(handle_, const_cast<char*>(fieldName.c_str()), value);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    void insertNew() {
        checkHandle();
        ::Errors err = ::insertNew(handle_);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    void insertAtBegin() {
        checkHandle();
        ::Errors err = ::insertaNew(handle_);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    void insertAtEnd() {
        checkHandle();
        ::Errors err = ::insertzNew(handle_);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    // Удаление текущей записи
    void deleteRecord() {
        checkHandle();
        ::Errors err = ::deleteRec(handle_);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
    }
    
    // Информация о таблице
    unsigned getFieldCount() const {
        checkHandle();
        unsigned num = 0;
        ::Errors err = ::getFieldsNum(handle_, &num);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
        return num;
    }
    
    std::string getFieldName(unsigned index) const {
        checkHandle();
        char* name = nullptr;
        ::Errors err = ::getFieldName(handle_, index, &name);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
        return std::string(name);
    }
    
    FieldType getFieldType(const std::string& fieldName) const {
        checkHandle();
        ::FieldType type;
        ::Errors err = ::getFieldType(handle_, const_cast<char*>(fieldName.c_str()), &type);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
        return static_cast<FieldType>(type);
    }
    
    unsigned getFieldLength(const std::string& fieldName) const {
        checkHandle();
        unsigned len = 0;
        ::Errors err = ::getFieldLen(handle_, const_cast<char*>(fieldName.c_str()), &len);
        if (err != ::OK) {
            throw TableException(getErrorString(err));
        }
        return len;
    }
    
    // Проверка состояния
    bool isOpen() const { return handle_ != nullptr; }

private:
    void checkHandle() const {
        if (!handle_) {
            throw TableException("Table is not open");
        }
    }
    
    ::THandle handle_;
};

} // namespace TableLib

#endif // TABLE_WRAPPER_H
