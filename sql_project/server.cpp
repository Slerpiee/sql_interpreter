/*
 * server.cpp
 * SQL Server process.
 * Reads SQL queries from stdin (framed by Protocol), executes them,
 * and writes results back to stdout (framed).
 *
 * Architecture: single-client, co-located.
 * The server is spawned as a child process by the client.
 */

#include "SQLServer.hpp"
#include "Protocol.hpp"
#include "ResultFormatter.hpp"
#include <iostream>
#include <string>
#include <unistd.h>

int main() {
    SQLServer server;

    // Redirect stdin/stdout for pipe IPC; stderr stays for logging
    int rfd = STDIN_FILENO;
    int wfd = STDOUT_FILENO;

    std::cerr << "[Server] Started, PID=" << getpid() << "\n";

    while (true) {
        std::string sql;
        try {
            sql = proto_read(rfd);
        } catch (const ProtocolException& e) {
            // client closed the pipe – normal shutdown
            std::cerr << "[Server] Client disconnected: " << e.what() << "\n";
            break;
        }

        if (sql == "\\quit") {
            std::cerr << "[Server] Quit received.\n";
            break;
        }

        std::cerr << "[Server] Query: " << sql << "\n";

        std::string response;
        try {
            ResultSet rs = server.execute(sql);
            response = ResultFormatter::serialize(rs);
        } catch (const ParseException& e) {
            response = ResultFormatter::serializeError(e.what());
        } catch (const TableException& e) {
            response = ResultFormatter::serializeError(e.what());
        } catch (const SQLExecutionException& e) {
            response = ResultFormatter::serializeError(e.what());
        } catch (const std::exception& e) {
            response = ResultFormatter::serializeError(std::string("Internal error: ") + e.what());
        }

        try {
            proto_write(wfd, response);
        } catch (const ProtocolException& e) {
            std::cerr << "[Server] Write failed: " << e.what() << "\n";
            break;
        }
    }

    std::cerr << "[Server] Exiting.\n";
    return 0;
}
