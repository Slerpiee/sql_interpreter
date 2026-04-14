#ifndef TABLE_TYPES_H
#define TABLE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Basic types */
typedef enum { FALSE = 0, TRUE } Bool;

/* Field types */
enum FieldType {
    Long,
    Text
};

/* Error codes */
enum Errors {
    OK = 0,
    CantCreateTable,
    CantOpenTable,
    FieldNotFound,
    BadHandle,
    BadArgs,
    CantMoveToPos,
    FileWriteError,
    FileReadError,
    CorruptedData,
    CantCreateHandle,
    ReadOnlyFile,
    BadFileName,
    CantDeleteTable,
    FileDataCorrupted,
    BadFilePos,
    BadFieldType,
    BadFieldLen,
    NoEditing,
    BadPos
};

/* Maximum field name length */
#define MaxFieldNameLen 64

/* Field definition structure */
struct FieldDef {
    char name[MaxFieldNameLen];
    enum FieldType type;
    unsigned len;  /* used only for Text fields */
};

/* Table structure for table creation */
struct TableStruct {
    long numOfFields;
    struct FieldDef *fieldsDef;
};

/* Opaque handle to table */
typedef struct Table* THandle;

/* C API functions from Table.c */
extern enum Errors createTable(char *tableName, struct TableStruct *tableStruct);
extern enum Errors deleteTable(char *tableName);
extern enum Errors openTable(char *tableName, THandle *tableHandle);
extern enum Errors closeTable(THandle tableHandle);
extern enum Errors moveFirst(THandle tableHandle);
extern enum Errors moveLast(THandle tableHandle);
extern enum Errors moveNext(THandle tableHandle);
extern enum Errors movePrevios(THandle tableHandle);
extern Bool beforeFirst(THandle tableHandle);
extern Bool afterLast(THandle tableHandle);
extern enum Errors getText(THandle tableHandle, char *fieldName, char **pvalue);
extern enum Errors getLong(THandle tableHandle, char *fieldName, long *pvalue);
extern enum Errors startEdit(THandle tableHandle);
extern enum Errors putText(THandle tableHandle, char *fieldName, char *value);
extern enum Errors putLong(THandle tableHandle, char *fieldName, long value);
extern enum Errors finishEdit(THandle tableHandle);
extern enum Errors createNew(THandle tableHandle);
extern enum Errors putTextNew(THandle tableHandle, char *fieldName, char *value);
extern enum Errors putLongNew(THandle tableHandle, char *fieldName, long value);
extern enum Errors insertNew(THandle tableHandle);
extern enum Errors insertaNew(THandle tableHandle);
extern enum Errors insertzNew(THandle tableHandle);
extern char *getErrorString(enum Errors code);
extern enum Errors getFieldLen(THandle tableHandle, char *fieldName, unsigned *plen);
extern enum Errors getFieldType(THandle tableHandle, char *fieldName, enum FieldType *ptype);
extern enum Errors getFieldsNum(THandle tableHandle, unsigned *pNum);
extern enum Errors getFieldName(THandle tableHandle, unsigned index, char **pFieldName);
extern enum Errors deleteRec(THandle tableHandle);

#ifdef __cplusplus
}
#endif

#endif /* TABLE_TYPES_H */
