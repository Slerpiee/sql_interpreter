#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"
#include <string>

// SQL Клиент
class SQLClient {
public:
    SQLClient();
    ~SQLClient();
    
    // Подключение к серверу (в данной реализации - просто создание сервера)
    bool connect(const std::string& serverAddress = "localhost", int port = 0);
    
    // Отключение от сервера
    void disconnect();
    
    // Отправка запроса на сервер
    ServerResponse sendQuery(const std::string& sqlQuery);
    
    // Проверка подключения
    bool isConnected() const { return connected; }
    
    // Получить последнюю ошибку
    std::string getLastError() const { return lastError; }
    
private:
    bool connected;
    std::string lastError;
    std::unique_ptr<SQLServer> server;
};

#endif // CLIENT_H
