#include "Table.hpp"
#include <iostream>
#include <cassert>
#include <string>

void testBasicOperations() {
    std::cout << "Testing basic operations..." << std::endl;
    
    // Создаем таблицу с двумя полями: "name" (строка) и "age" (число)
    const char* fields[] = {"name", "age"};
    table::Table tbl = table::Table::create("test_table.db", 2, fields);
    
    // Проверяем количество полей
    assert(tbl.getFieldCount() == 2);
    assert(tbl.getFieldName(0) == "name");
    assert(tbl.getFieldName(1) == "age");
    
    // Вставляем первую запись
    tbl.createNew();
    tbl.putTextNew(0, "Alice");
    tbl.putLongNew(1, 25);
    tbl.insertNew();
    
    // Вставляем вторую запись
    tbl.createNew();
    tbl.putTextNew(0, "Bob");
    tbl.putLongNew(1, 30);
    tbl.insertNew();
    
    // Перемещаемся в начало и проверяем данные
    tbl.moveFirst();
    assert(!tbl.isBeforeFirst());
    assert(tbl.getText(0) == "Alice");
    assert(tbl.getLong(1) == 25);
    
    // Перемещаемся к следующей записи
    tbl.moveNext();
    assert(tbl.getText(0) == "Bob");
    assert(tbl.getLong(1) == 30);
    
    // Проверяем, что после последней записи курсор указывает на конец
    assert(tbl.moveNext() == false);
    assert(tbl.isAfterLast());
    
    // Редактирование записи
    tbl.moveFirst();
    tbl.startEdit();
    tbl.putText(0, "Alicia");
    tbl.finishEdit();
    
    tbl.moveFirst();
    assert(tbl.getText(0) == "Alicia");
    
    // Удаление записи
    tbl.moveFirst();
    tbl.deleteRecord();
    
    // Теперь первая запись должна быть "Bob"
    tbl.moveFirst();
    assert(tbl.getText(0) == "Bob");
    
    std::cout << "Basic operations test passed!" << std::endl;
}

void testNavigation() {
    std::cout << "Testing navigation..." << std::endl;
    
    const char* fields[] = {"id"};
    table::Table tbl = table::Table::create("nav_test.db", 1, fields);
    
    // Вставляем несколько записей
    for (int i = 1; i <= 5; ++i) {
        tbl.createNew();
        tbl.putLongNew(0, i * 10);
        tbl.insertNew();
    }
    
    // Движемся вперед
    tbl.moveFirst();
    assert(tbl.getLong(0) == 10);
    
    tbl.moveNext();
    assert(tbl.getLong(0) == 20);
    
    tbl.moveNext();
    assert(tbl.getLong(0) == 30);
    
    // Движемся назад
    tbl.movePrevious();
    assert(tbl.getLong(0) == 20);
    
    tbl.movePrevious();
    assert(tbl.getLong(0) == 10);
    
    // Проверка isBeforeFirst
    assert(tbl.movePrevious() == false);
    assert(tbl.isBeforeFirst());
    
    // Движемся в конец
    tbl.moveLast();
    assert(tbl.getLong(0) == 50);
    assert(!tbl.isAfterLast());
    
    std::cout << "Navigation test passed!" << std::endl;
}

void testExceptions() {
    std::cout << "Testing exceptions..." << std::endl;
    
    const char* fields[] = {"data"};
    table::Table tbl = table::Table::create("exc_test.db", 1, fields);
    
    bool caughtException = false;
    try {
        // Попытка чтения без активной записи
        tbl.moveFirst(); // Таблица пуста, курсор before first
        tbl.getText(0);
    } catch (const table::TableException& e) {
        caughtException = true;
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
    assert(caughtException);
    
    caughtException = false;
    try {
        // Попытка редактирования без startEdit
        tbl.createNew();
        tbl.putTextNew(0, "test");
        tbl.insertNew();
        tbl.moveFirst();
        tbl.putText(0, "modified"); // Ошибка: нет startEdit
    } catch (const table::TableException& e) {
        caughtException = true;
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
    assert(caughtException);
    
    std::cout << "Exceptions test passed!" << std::endl;
}

int main() {
    try {
        testBasicOperations();
        testNavigation();
        testExceptions();
        
        std::cout << "\n=== All tests passed successfully! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
