#include "server.h"

SQLServer::SQLServer() : totalRequests(0) {}

SQLServer::~SQLServer() {}

ServerResponse SQLServer::processRequest(const std::string& sqlQuery) {
    totalRequests++;
    
    ServerResponse response;
    
    if (sqlQuery.empty()) {
        return ServerResponse::error(ServerErrorCode::PARSE_ERROR, "Empty query");
    }
    
    // Выполняем запрос через DatabaseManager
    SQLResult result = dbManager.execute(sqlQuery);
    
    if (!result.success) {
        response.errorCode = ServerErrorCode::EXECUTION_ERROR;
        response.errorMessage = result.errorMessage;
        return response;
    }
    
    response.result = result;
    return response;
}
