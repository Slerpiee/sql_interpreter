#include "ui.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

UserInterface::UserInterface() : running(false) {}

UserInterface::~UserInterface() {
    stop();
}

void UserInterface::run() {
    if (!client.connect()) {
        std::cerr << "Failed to connect to server\n";
        return;
    }
    
    running = true;
    printWelcome();
    
    while (running) {
        std::cout << "\nsql> ";
        std::string command = readCommand();
        
        if (command.empty()) continue;
        
        // Проверяем специальные команды
        if (handleSpecialCommand(command)) {
            continue;
        }
        
        // Отправляем запрос на сервер
        ServerResponse response = client.sendQuery(command);
        
        // Выводим результат
        printResult(response);
    }
    
    client.disconnect();
}

void UserInterface::stop() {
    running = false;
}

void UserInterface::printWelcome() {
    std::cout << "========================================\n";
    std::cout << "   Model SQL Interpreter v1.0\n";
    std::cout << "========================================\n";
    std::cout << "Type 'help' for available commands.\n";
    std::cout << "Type 'exit' or 'quit' to exit.\n";
}

void UserInterface::printHelp() {
    std::cout << "\nAvailable commands:\n";
    std::cout << "  help              - Show this help message\n";
    std::cout << "  exit/quit         - Exit the program\n";
    std::cout << "  clear             - Clear screen\n";
    std::cout << "\nSupported SQL statements:\n";
    std::cout << "  CREATE TABLE      - Create a new table\n";
    std::cout << "  DROP TABLE        - Delete a table\n";
    std::cout << "  INSERT INTO       - Insert data into table\n";
    std::cout << "  SELECT            - Query data from table\n";
    std::cout << "  UPDATE            - Update data in table\n";
    std::cout << "  DELETE FROM       - Delete data from table\n";
    std::cout << "\nExamples:\n";
    std::cout << "  CREATE TABLE users (id INTEGER, name VARCHAR(50));\n";
    std::cout << "  INSERT INTO users (id, name) VALUES (1, 'John');\n";
    std::cout << "  SELECT * FROM users;\n";
    std::cout << "  SELECT name FROM users WHERE id = 1;\n";
    std::cout << "  UPDATE users SET name = 'Jane' WHERE id = 1;\n";
    std::cout << "  DELETE FROM users WHERE id = 1;\n";
}

void UserInterface::printResult(const ServerResponse& response) {
    if (response.errorCode != ServerErrorCode::OK) {
        std::cout << "Error: " << response.errorMessage << "\n";
        return;
    }
    
    const SQLResult& result = response.result;
    
    if (result.columns.empty()) {
        std::cout << "Query executed successfully.\n";
        return;
    }
    
    if (result.rows.empty()) {
        std::cout << "(0 rows)\n";
        return;
    }
    
    // Вычисляем ширину колонок
    std::vector<size_t> colWidths(result.columns.size());
    for (size_t i = 0; i < result.columns.size(); i++) {
        colWidths[i] = result.columns[i].length();
    }
    
    for (const auto& row : result.rows) {
        for (size_t i = 0; i < result.columns.size(); i++) {
            const std::string& colName = result.columns[i];
            auto it = row.find(colName);
            std::string value;
            if (it != row.end()) {
                if (std::holds_alternative<int>(it->second)) {
                    value = std::to_string(std::get<int>(it->second));
                } else if (std::holds_alternative<std::string>(it->second)) {
                    value = std::get<std::string>(it->second);
                } else {
                    value = "NULL";
                }
            }
            if (value.length() > colWidths[i]) {
                colWidths[i] = value.length();
            }
        }
    }
    
    // Выводим заголовок
    std::cout << "\n";
    for (size_t i = 0; i < result.columns.size(); i++) {
        std::cout << " " << std::left << std::setw(colWidths[i]) << result.columns[i];
        if (i < result.columns.size() - 1) std::cout << " |";
    }
    std::cout << "\n";
    
    // Выводим разделитель
    for (size_t i = 0; i < result.columns.size(); i++) {
        for (size_t j = 0; j <= colWidths[i]; j++) std::cout << "-";
        if (i < result.columns.size() - 1) std::cout << "-+-";
    }
    std::cout << "\n";
    
    // Выводим строки
    for (const auto& row : result.rows) {
        for (size_t i = 0; i < result.columns.size(); i++) {
            const std::string& colName = result.columns[i];
            auto it = row.find(colName);
            std::string value;
            if (it != row.end()) {
                if (std::holds_alternative<int>(it->second)) {
                    value = std::to_string(std::get<int>(it->second));
                } else if (std::holds_alternative<std::string>(it->second)) {
                    value = std::get<std::string>(it->second);
                } else {
                    value = "NULL";
                }
            }
            std::cout << " " << std::left << std::setw(colWidths[i]) << value;
            if (i < result.columns.size() - 1) std::cout << " |";
        }
        std::cout << "\n";
    }
    
    std::cout << "\n(" << result.rows.size() << " rows)\n";
}

std::string UserInterface::readCommand() {
    std::string line;
    std::getline(std::cin, line);
    
    // Удаляем завершающую точку с запятой если есть
    if (!line.empty() && line.back() == ';') {
        line.pop_back();
    }
    
    // Trim whitespace
    size_t start = line.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = line.find_last_not_of(" \t\n\r");
    return line.substr(start, end - start + 1);
}

bool UserInterface::handleSpecialCommand(const std::string& cmd) {
    std::string upperCmd = cmd;
    std::transform(upperCmd.begin(), upperCmd.end(), upperCmd.begin(), ::toupper);
    
    if (upperCmd == "HELP" || upperCmd == "?") {
        printHelp();
        return true;
    }
    
    if (upperCmd == "EXIT" || upperCmd == "QUIT") {
        running = false;
        return true;
    }
    
    if (upperCmd == "CLEAR" || upperCmd == "CLS") {
        system("clear");
        return true;
    }
    
    return false;
}
