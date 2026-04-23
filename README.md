# TableLib - C++ обертка для Си-библиотеки работы с таблицами

## Описание

Этот проект предоставляет объектно-ориентированный C++ интерфейс к библиотеке на Си 
для работы с табличными данными (файл `Table.c`).

## Файлы проекта

- `_Table.h` - Заголовочный файл Си-библиотеки с определениями типов и функций
- `Table.c` - Исходный код Си-библиотеки
- `TableWrapper.h` - C++ класс-обертка для ООП-стиля программирования
- `test_table.cpp` - Пример использования библиотеки

## Классы

### TableLib::TableException
Класс исключения для обработки ошибок работы с таблицей.
Наследуется от `std::runtime_error`.

### TableLib::FieldType (enum class)
Перечисление типов полей:
- `Long` - целочисленное поле
- `Text` - текстовое поле

### TableLib::ErrorCode (enum class)
Перечисление кодов ошибок (OK, CantCreateTable, FieldNotFound, и т.д.)

### TableLib::FieldDefinition
Структура для определения поля таблицы:
```cpp
FieldDefinition(const std::string& name, FieldType type, unsigned long length)
```

### TableLib::Table
Основной класс для работы с таблицами.

#### Статические методы
- `static void create(const std::string& tableName, const std::vector<FieldDefinition>& fields)` - создать новую таблицу
- `static void drop(const std::string& tableName)` - удалить таблицу из файловой системы

#### Методы управления
- `void open(const std::string& tableName)` - открыть существующую таблицу
- `void close()` - закрыть таблицу
- `bool isOpen() const` - проверить, открыта ли таблица

#### Навигация по записям
- `void moveFirst()` - перейти к первой записи
- `void moveLast()` - перейти к последней записи
- `void moveNext()` - перейти к следующей записи
- `void movePrevious()` - перейти к предыдущей записи
- `bool isBeforeFirst() const` - проверка позиции перед первой записью
- `bool isAfterLast() const` - проверка позиции после последней записи

#### Чтение данных
- `std::string getText(const std::string& fieldName)` - получить текстовое значение
- `long getLong(const std::string& fieldName)` - получить числовое значение

#### Редактирование записей
- `void startEdit()` - начать редактирование текущей записи
- `void putText(const std::string& fieldName, const std::string& value)` - установить текстовое значение
- `void putLong(const std::string& fieldName, long value)` - установить числовое значение
- `void finishEdit()` - завершить редактирование и сохранить изменения

#### Вставка новых записей
- `void createNew()` - инициализировать новую запись
- `void putTextNew(const std::string& fieldName, const std::string& value)` - установить текст для новой записи
- `void putLongNew(const std::string& fieldName, long value)` - установить число для новой записи
- `void insertNew()` - вставить новую запись в текущую позицию
- `void insertAtBegin()` - вставить новую запись в начало
- `void insertAtEnd()` - вставить новую запись в конец

#### Удаление записей
- `void deleteRecord()` - удалить текущую запись

#### Информация о таблице
- `unsigned getFieldCount() const` - получить количество полей
- `std::string getFieldName(unsigned index) const` - получить имя поля по индексу
- `FieldType getFieldType(const std::string& fieldName) const` - получить тип поля
- `unsigned getFieldLength(const std::string& fieldName) const` - получить длину поля

## Пример использования

```cpp
#include <iostream>
#include "TableWrapper.h"

int main() {
    try {
        // Создаем таблицу с полями
        std::vector<TableLib::FieldDefinition> fields = {
            TableLib::FieldDefinition("id", TableLib::FieldType::Long, sizeof(long)),
            TableLib::FieldDefinition("name", TableLib::FieldType::Text, 50),
            TableLib::FieldDefinition("email", TableLib::FieldType::Text, 100)
        };
        
        TableLib::Table::create("users.dat", fields);
        
        // Открываем таблицу
        TableLib::Table table;
        table.open("users.dat");
        
        // Добавляем запись
        table.createNew();
        table.putLongNew("id", 1);
        table.putTextNew("name", "Alice");
        table.putTextNew("email", "alice@example.com");
        table.insertAtEnd();
        
        // Читаем записи
        table.moveFirst();
        while (!table.isAfterLast()) {
            std::cout << table.getText("name") << std::endl;
            table.moveNext();
        }
        
        table.close();
        TableLib::Table::drop("users.dat");
        
    } catch (const TableLib::TableException& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
    
    return 0;
}
```

## Компиляция

```bash
# Компиляция Си-библиотеки
gcc -c Table.c -o Table.o

# Компиляция тестового файла
g++ -c test_table.cpp -o test_table.o

# Линковка
g++ test_table.o Table.o -o test_table

# Запуск
./test_table
```

## Особенности

- Класс `Table` использует RAII для автоматического закрытия таблицы при уничтожении
- Копирование объектов запрещено, разрешено только перемещение (move semantics)
- Все ошибки выбрасываются как исключения типа `TableException`
- Обертка полностью скрывает работу с Си-указателями и ручное управление памятью
