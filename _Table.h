#ifndef _TABLE_H
#define _TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#define MaxFieldNameLen 64

/* Boolean type */
typedef enum {
    FALSE = 0,
    TRUE = 1
} Bool;

/* Field types */
enum FieldType {
    Long,
    Text
};

/* Error codes */
enum Errors {
    OK,
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
    BadFilePosition,
    BadFieldType,
    BadFieldLen,
    NoEditing,
    BadPos
};

/* Aliases for compatibility with Table.c */
#define BadPosition BadFilePosition
#define CantWriteData FileWriteError
#define CantReadData FileReadError
#define CorruptedFile CorruptedData

/* Field definition structure */
struct FieldDefinition {
    char fieldName[MaxFieldNameLen];
    enum FieldType type;
    long len;
};

/* Alias for compatibility with Table.c which uses 'name' field */
struct FieldDefCompat {
    char name[MaxFieldNameLen];
    enum FieldType type;
    long len;
};

/* Table structure for table creation */
struct TableStruct {
    long numOfFields;
    struct FieldDefinition* fieldsDef;
};

/* Opaque table handle type */
typedef struct Table* THandle;

/* Function prototypes */
enum Errors createTable(char *tableName, struct TableStruct *tableStruct);
enum Errors deleteTable(char *tableName);
enum Errors openTable(char *tableName, THandle *tableHandle);
enum Errors closeTable(THandle tableHandle);

/* Navigation functions */
enum Errors moveFirst(THandle tableHandle);
enum Errors moveLast(THandle tableHandle);
enum Errors moveNext(THandle tableHandle);
enum Errors movePrevios(THandle tableHandle);
Bool beforeFirst(THandle tableHandle);
Bool afterLast(THandle tableHandle);

/* Get field values */
enum Errors getText(THandle tableHandle, char *fieldName, char **pvalue);
enum Errors getLong(THandle tableHandle, char *fieldName, long *pvalue);

/* Edit current record */
enum Errors startEdit(THandle tableHandle);
enum Errors putText(THandle tableHandle, char *fieldName, char *value);
enum Errors putLong(THandle tableHandle, char *fieldName, long value);
enum Errors finishEdit(THandle tableHandle);

/* Create and insert new record */
enum Errors createNew(THandle tableHandle);
enum Errors putTextNew(THandle tableHandle, char *fieldName, char *value);
enum Errors putLongNew(THandle tableHandle, char *fieldName, long value);
enum Errors insertNew(THandle tableHandle);
enum Errors insertaNew(THandle tableHandle);
enum Errors insertzNew(THandle tableHandle);

/* Delete record */
enum Errors deleteRec(THandle tableHandle);

/* Get table/field info */
char *getErrorString(enum Errors code);
enum Errors getFieldLen(THandle tableHandle, char *fieldName, unsigned *plen);
enum Errors getFieldType(THandle tableHandle, char *fieldName, enum FieldType *ptype);
enum Errors getFieldsNum(THandle tableHandle, unsigned *pNum);
enum Errors getFieldName(THandle tableHandle, unsigned index, char **pFieldName);

#ifdef __cplusplus
}
#endif

#endif /* _TABLE_H */
