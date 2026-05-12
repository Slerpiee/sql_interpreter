#include "sock_wrap.h"
#include <iostream>
#include <string>

using namespace SQL;

int main() {
    const std::string socketPath = "/tmp/sql_server.sock";
    std::cout << "🔌 Connecting to server..." << std::endl;
    ClientSocket client(socketPath);
    std::cout << "✅ Connected. Type SQL queries (Ctrl+D to exit):\n";
    
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        try {
            client.sendString(line);
            std::string response = client.receiveString();
            std::cout << response << "\n\n";
        } catch (const std::exception& e) {
            std::cerr << "❌ " << e.what() << std::endl;
            break;
        }
    }
    std::cout << "👋 Client disconnected.\n";
    return 0;
}
