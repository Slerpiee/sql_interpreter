#include "interpreter.h"
#include "sock_wrap.h"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace SQL;

int main() {
    const std::string socketPath = "/tmp/sql_server.sock";
    Interpreter interp("."); // БД в текущей папке
    
    try {
        ServerSocket server(socketPath);
        std::cout << "🟢 Server listening on " << socketPath << std::endl;
        
        while (true) {
            int clientFd = server.acceptConnection();
            std::cout << "🔹 Client connected" << std::endl;
            
            char buf[4096];
            ssize_t n = read(clientFd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                std::string query(buf);
                if (!query.empty() && query.back() == '\n') query.pop_back();
                
                std::cout << "📥 Query: " << query << std::endl;
                
                try {
                    auto cmd = parseSQL(query);
                    auto result = interp.execute(cmd);
                    
                    std::ostringstream response;
                    if (!result.success) {
                        response << "ERROR: " << result.message;
                    } else if (result.rows.empty()) {
                        response << "OK: " << result.message;
                    } else {
                        response << "RESULT:\n";
                        // Простой табличный вывод
                        for (auto& row : result.rows) {
                            for (auto& col : row) response << std::left << std::setw(15) << col;
                            response << "\n";
                        }
                    }
                    std::string resp = response.str();
                    write(clientFd, resp.c_str(), resp.size());
                    write(clientFd, "\n", 1);
                } catch (const std::exception& e) {
                    std::string err = "ERROR: " + std::string(e.what()) + "\n";
                    write(clientFd, err.c_str(), err.size());
                }
            }
            close(clientFd);
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
