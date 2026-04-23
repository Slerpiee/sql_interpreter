#ifndef TABLE_LIB_H
#define TABLE_LIB_H

// Библиотека работы с таблицами (симуляция на основе описания)
// Эти определения нужны для понимания типов при проектировании AST

#include <cstddef>
#include <cstdint>

// Максимальная длина имени поля
const unsigned MaxFieldNameLen = 64;

// Сигнатура файла таблицы
const char FILE_SIGNATURE[] = "\033\032DataFile\033\032~~~";

// Типы полей
enum FieldType { 
    Text, 
    Long 
};

// Коды ошибок
enum Errors {
    OK = 0,
    CantCreateTable,
    CantOpenTable,
    FieldNotFound,
    BadHandle,
    BadArgs,
    CantMoveToPos,
    CantWriteData,
    CantReadData,
    CorruptedFile,
    BadFieldType,
    BadFieldLen,
    BadPosition,
    TableNotFound,
    FileError,
    MemoryError
};

// Текстовое представление ошибки
const char* ErrorText(Errors err);

// Структура поля в определении таблицы
struct FieldDef {
    char name[MaxFieldNameLen];
    enum FieldType type;
    unsigned len;  // для Text полей — длина строки, для Long не используется
};

// Структура для createTable
struct TableStruct {
    unsigned numOfFields;
    struct FieldDef *fieldsDef;
};

// Дескриптор таблицы (непрозрачный указатель)
typedef struct Table* THandle;

// API для работы с таблицами
#ifdef __cplusplus
extern "C" {
#endif

// Создание/удаление/открытие таблиц
Errors createTable(char *tableName, struct TableStruct *tableStruct);
Errors deleteTable(char *tableName);
Errors openTable(char *tableName, THandle *tableHandle);
Errors closeTable(THandle tableHandle);

// Навигация по записям (скрытый курсор)
Errors moveFirst(THandle tableHandle);
Errors moveLast(THandle tableHandle);
Errors moveNext(THandle tableHandle);
Errors movePrev(THandle tableHandle);
Errors moveBeforeFirst(THandle tableHandle);
Errors moveAfterLast(THandle tableHandle);

// Чтение значений полей текущей записи
Errors getText(THandle tableHandle, char *fieldName, char **value);
Errors getLong(THandle tableHandle, char *fieldName, long *value);

// Установка новых значений (для insert/update)
Errors putTextNew(THandle tableHandle, char *fieldName, char *value);
Errors putLongNew(THandle tableHandle, char *fieldName, long value);
Errors putTextEdit(THandle tableHandle, char *fieldName, char *value);
Errors putLongEdit(THandle tableHandle, char *fieldName, long value);

// Вставка новой записи
Errors insertNew(THandle tableHandle);
Errors insertBeforeFirst(THandle tableHandle);
Errors insertAfterLast(THandle tableHandle);

// Обновление/удаление текущей записи
Errors updateRec(THandle tableHandle);
Errors deleteRec(THandle tableHandle);

// Получение метаданных таблицы
Errors getFieldsNum(THandle tableHandle, unsigned *pNum);
Errors getFieldName(THandle tableHandle, unsigned index, char **pFieldName);

// Проверка конца/начала таблицы
int beforeFirst(THandle tableHandle);
int afterLast(THandle tableHandle);

#ifdef __cplusplus
}
#endif

#endif // TABLE_LIB_H
