#include "client.h"

SQLClient::SQLClient() : connected(false) {}

SQLClient::~SQLClient() {
    disconnect();
}

bool SQLClient::connect(const std::string& /*serverAddress*/, int /*port*/) {
    if (connected) {
        return true;
    }
    
    server = std::make_unique<SQLServer>();
    connected = true;
    lastError.clear();
    
    return true;
}

void SQLClient::disconnect() {
    server.reset();
    connected = false;
}

ServerResponse SQLClient::sendQuery(const std::string& sqlQuery) {
    if (!connected) {
        ServerResponse resp;
        resp.errorCode = ServerErrorCode::INTERNAL_ERROR;
        resp.errorMessage = "Not connected to server";
        return resp;
    }
    
    // Клиент передает запрос серверу без анализа
    ServerResponse response = server->processRequest(sqlQuery);
    
    if (response.errorCode != ServerErrorCode::OK) {
        lastError = response.errorMessage;
    } else {
        lastError.clear();
    }
    
    return response;
}
