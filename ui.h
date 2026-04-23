#ifndef UI_H
#define UI_H

#include "client.h"
#include <string>
#include <vector>

// Класс пользовательского интерфейса
class UserInterface {
public:
    UserInterface();
    ~UserInterface();
    
    // Запуск интерфейса
    void run();
    
    // Остановка интерфейса
    void stop();
    
private:
    SQLClient client;
    bool running;
    
    // Вывод приветственного сообщения
    void printWelcome();
    
    // Вывод подсказки
    void printHelp();
    
    // Вывод результата запроса в табличном виде
    void printResult(const ServerResponse& response);
    
    // Чтение команды от пользователя
    std::string readCommand();
    
    // Обработка специальных команд
    bool handleSpecialCommand(const std::string& cmd);
};

#endif // UI_H
