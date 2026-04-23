#ifndef SERVER_H
#define SERVER_H

#include "database.h"
#include <string>
#include <memory>

// Коды ошибок сервера
enum class ServerErrorCode {
    OK = 0,
    PARSE_ERROR,
    EXECUTION_ERROR,
    INTERNAL_ERROR
};

// Структура ответа сервера
struct ServerResponse {
    ServerErrorCode errorCode;
    std::string errorMessage;
    SQLResult result;
    
    ServerResponse() : errorCode(ServerErrorCode::OK) {}
    
    static ServerResponse error(ServerErrorCode code, const std::string& msg) {
        ServerResponse resp;
        resp.errorCode = code;
        resp.errorMessage = msg;
        return resp;
    }
};

// SQL Сервер
class SQLServer {
public:
    SQLServer();
    ~SQLServer();
    
    // Обработка запроса от клиента
    ServerResponse processRequest(const std::string& sqlQuery);
    
    // Получить статистику
    int getTotalRequests() const { return totalRequests; }
    
private:
    DatabaseManager dbManager;
    int totalRequests;
};

#endif // SERVER_H
