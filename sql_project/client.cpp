#include "Protocol.hpp"
#include "ResultFormatter.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

struct PipePair { int r, w; };

static PipePair make_pipe() {
    int fd[2];
    if (pipe(fd) != 0) throw std::runtime_error("pipe() failed");
    return {fd[0], fd[1]};
}

class SQLClient {
public:
    SQLClient() {
        PipePair toServer   = make_pipe();
        PipePair fromServer = make_pipe();

        pid_ = fork();
        if (pid_ < 0) throw std::runtime_error("fork() failed");

        if (pid_ == 0) {
            close(toServer.w);
            close(fromServer.r);

            if (dup2(toServer.r,   STDIN_FILENO)  < 0) { perror("dup2"); _exit(1); }
            if (dup2(fromServer.w, STDOUT_FILENO) < 0) { perror("dup2"); _exit(1); }

            close(toServer.r);
            close(fromServer.w);

            execl("./server", "./server", nullptr);
            perror("execl");
            _exit(1);
        }

        close(toServer.r);
        close(fromServer.w);
        wfd_ = toServer.w;
        rfd_ = fromServer.r;
    }

    ~SQLClient() {
        try { sendSQL("\\quit"); } catch (...) {}
        close(wfd_);
        close(rfd_);
        waitpid(pid_, nullptr, 0);
    }

    std::string query(const std::string& sql) {
        sendSQL(sql);
        std::string payload = proto_read(rfd_);

        ResultSet rs;
        std::string errMsg;
        if (!ResultFormatter::deserialize(payload, rs, errMsg)) {
            return "ERROR: " + errMsg + "\n";
        }
        return ResultFormatter::prettyPrint(rs);
    }

private:
    pid_t pid_;
    int   wfd_, rfd_;

    void sendSQL(const std::string& sql) {
        proto_write(wfd_, sql);
    }
};

static const char* HELP = R"(
Model SQL Interpreter  тАУ  supported statements:
  CREATE TABLE <t> (<col> LONG | TEXT(<n>) [, ...])
  DROP TABLE <t>
  INSERT INTO <t> (<col>, ...) VALUES (<val>, ...)
  SELECT * | <col>, ...  FROM <t>  [WHERE <cond>]
  UPDATE <t> SET <col>=<val> [, ...]  [WHERE <cond>]
  DELETE FROM <t>  [WHERE <cond>]

WHERE condition syntax:
  <col> = | <> | < | > | <= | >= <literal>
  <cond> AND | OR <cond>
  NOT <cond>   |   (<cond>)

String literals are quoted with single quotes: 'hello'
Long literals are plain integers: 42

Special client commands:
  \help   тАУ show this help
  \quit   тАУ exit
)";

int main() {
    std::cout << "Model SQL Client\n";
    std::cout << "Type \\help for syntax help, \\quit to exit.\n\n";

    SQLClient client;

    std::string line, sql;
    while (true) {
        std::cout << (sql.empty() ? "sql> " : "  -> ");
        std::cout.flush();

        if (!std::getline(std::cin, line)) break;

        if (!sql.empty()) sql += " ";
        sql += line;

        {
            std::string trimmed = sql;
            while (!trimmed.empty() && std::isspace((unsigned char)trimmed.back())) trimmed.pop_back();
            if (trimmed == "\\quit") break;
            if (trimmed == "\\help") { std::cout << HELP; sql.clear(); continue; }
        }

        if (sql.find(';') == std::string::npos &&
            sql.find("\\quit") == std::string::npos) {
            continue;
        }

        while (!sql.empty() && (sql.back()==';'||std::isspace((unsigned char)sql.back())))
            sql.pop_back();

        if (sql.empty()) continue;

        try {
            std::string result = client.query(sql);
            std::cout << result;
        } catch (const std::exception& e) {
            std::cerr << "Client error: " << e.what() << "\n";
        }
        sql.clear();
    }

    std::cout << "\nGoodbye.\n";
    return 0;
}
