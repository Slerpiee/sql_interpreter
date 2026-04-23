#ifndef _TABLE_H
#define _TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#define MaxFieldNameLen 32

typedef enum { FALSE = 0, TRUE } Bool;

typedef enum { Long, Text } FieldType;

typedef enum {
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
    CantCreateHandle,
    ReadOnlyFile,
    BadFileName,
    CantDeleteTable,
    CorruptedData,
    BadPos,
    BadFieldType,
    BadFieldLen,
    NoEditing,
    BadPosition
} Errors;

typedef struct {
    char name[MaxFieldNameLen];
    FieldType type;
    int len;
} FieldDef;

typedef struct TableStruct {
    int numOfFields;
    FieldDef *fieldsDef;
} TableStruct;

typedef struct Table *THandle;

/* Function prototypes */
Errors createTable(char *tableName, TableStruct *tableStruct);
Errors deleteTable(char *tableName);
Errors openTable(char *tableName, THandle *tableHandle);
Errors closeTable(THandle tableHandle);
Errors moveFirst(THandle tableHandle);
Errors moveLast(THandle tableHandle);
Errors moveNext(THandle tableHandle);
Errors movePrevios(THandle tableHandle);
Bool beforeFirst(THandle tableHandle);
Bool afterLast(THandle tableHandle);
Errors getText(THandle tableHandle, char *fieldName, char **pvalue);
Errors getLong(THandle tableHandle, char *fieldName, long *pvalue);
Errors startEdit(THandle tableHandle);
Errors putText(THandle tableHandle, char *fieldName, char *value);
Errors putLong(THandle tableHandle, char *fieldName, long value);
Errors finishEdit(THandle tableHandle);
Errors createNew(THandle tableHandle);
Errors putTextNew(THandle tableHandle, char *fieldName, char *value);
Errors putLongNew(THandle tableHandle, char *fieldName, long value);
Errors insertNew(THandle tableHandle);
Errors insertaNew(THandle tableHandle);
Errors insertzNew(THandle tableHandle);
char *getErrorString(Errors code);
Errors getFieldLen(THandle tableHandle, char *fieldName, unsigned *plen);
Errors getFieldType(THandle tableHandle, char *fieldName, FieldType *ptype);
Errors getFieldsNum(THandle tableHandle, unsigned *pNum);
Errors getFieldName(THandle tableHandle, unsigned index, char **pFieldName);
Errors deleteRec(THandle tableHandle);

#ifdef __cplusplus
}
#endif

#endif /* _TABLE_H */
