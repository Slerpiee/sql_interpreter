#include <iostream>
#include "TableWrapper.h"

int main() {
    const std::string tableName = "test.dat";
    
    try {
        // Удаляем таблицу если существует (игнорируем ошибки)
        try {
            TableLib::Table::drop(tableName);
        } catch (...) {}
        
        // Создаем таблицу с полями: id (Long), name (Text), description (Text)
        std::cout << "=== Создание таблицы ===" << std::endl;
        std::vector<TableLib::FieldDefinition> fields = {
            TableLib::FieldDefinition("id", TableLib::FieldType::Long, sizeof(long)),
            TableLib::FieldDefinition("name", TableLib::FieldType::Text, 50),
            TableLib::FieldDefinition("description", TableLib::FieldType::Text, 100)
        };
        
        TableLib::Table::create(tableName, fields);
        std::cout << "Таблица успешно создана!" << std::endl;
        
        // Открываем таблицу
        std::cout << "\n=== Открытие таблицы ===" << std::endl;
        TableLib::Table table;
        table.open(tableName);
        std::cout << "Таблица открыта. Количество полей: " << table.getFieldCount() << std::endl;
        
        // Выводим имена полей
        std::cout << "Поля таблицы: ";
        for (unsigned i = 0; i < table.getFieldCount(); ++i) {
            std::cout << table.getFieldName(i) << " ";
        }
        std::cout << std::endl;
        
        // Добавляем первую запись
        std::cout << "\n=== Добавление записей ===" << std::endl;
        table.createNew();
        table.putLongNew("id", 1);
        table.putTextNew("name", "Alice");
        table.putTextNew("description", "First user in the database");
        table.insertAtEnd();
        std::cout << "Запись 1 добавлена" << std::endl;
        
        // Добавляем вторую запись
        table.createNew();
        table.putLongNew("id", 2);
        table.putTextNew("name", "Bob");
        table.putTextNew("description", "Second user");
        table.insertAtEnd();
        std::cout << "Запись 2 добавлена" << std::endl;
        
        // Добавляем третью запись
        table.createNew();
        table.putLongNew("id", 3);
        table.putTextNew("name", "Charlie");
        table.putTextNew("description", "Third user in the system");
        table.insertAtEnd();
        std::cout << "Запись 3 добавлена" << std::endl;
        
        // Навигация и чтение записей
        std::cout << "\n=== Чтение записей ===" << std::endl;
        table.moveFirst();
        int recordNum = 1;
        while (!table.isAfterLast()) {
            std::cout << "Запись #" << recordNum << ":" << std::endl;
            std::cout << "  ID: " << table.getLong("id") << std::endl;
            std::cout << "  Name: " << table.getText("name") << std::endl;
            std::cout << "  Description: " << table.getText("description") << std::endl;
            
            table.moveNext();
            recordNum++;
        }
        
        // Редактирование записи
        std::cout << "\n=== Редактирование записи ===" << std::endl;
        table.moveFirst();
        table.moveNext(); // Переходим ко второй записи (Bob)
        std::cout << "До редактирования: " << table.getText("name") << std::endl;
        
        table.startEdit();
        table.putText("name", "Bobby");
        table.putText("description", "Updated description for Bob");
        table.finishEdit();
        
        std::cout << "После редактирования: " << table.getText("name") << std::endl;
        std::cout << "Новое описание: " << table.getText("description") << std::endl;
        
        // Удаление записи
        std::cout << "\n=== Удаление записи ===" << std::endl;
        table.moveFirst(); // Переходим к первой записи
        std::cout << "Удаляем запись с ID: " << table.getLong("id") << std::endl;
        table.deleteRecord();
        std::cout << "Запись удалена" << std::endl;
        
        // Проверяем оставшиеся записи
        std::cout << "\n=== Оставшиеся записи ===" << std::endl;
        table.moveFirst();
        recordNum = 1;
        while (!table.isAfterLast()) {
            std::cout << "Запись #" << recordNum << ": " 
                      << table.getText("name") << " (ID: " << table.getLong("id") << ")" << std::endl;
            table.moveNext();
            recordNum++;
        }
        
        // Закрываем таблицу
        table.close();
        std::cout << "\nТаблица закрыта" << std::endl;
        
        // Удаляем тестовую таблицу
        std::cout << "\n=== Удаление таблицы ===" << std::endl;
        TableLib::Table::drop(tableName);
        std::cout << "Таблица удалена" << std::endl;
        
        std::cout << "\n=== Все тесты пройдены успешно! ===" << std::endl;
        
    } catch (const TableLib::TableException& e) {
        std::cerr << "Ошибка TableException: " << e.what() << std::endl;
        // Очищаем тестовый файл в случае ошибки
        try {
            TableLib::Table::drop(tableName);
        } catch (...) {}
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка std::exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
