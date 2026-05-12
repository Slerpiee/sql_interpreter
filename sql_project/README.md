# Model SQL Interpreter — C++ / Client-Server

## Архитектура

```
┌─────────────────────────────────────────────────────────┐
│                     Один компьютер                       │
│                                                         │
│  ┌──────────┐   pipe (SQL)    ┌──────────────────────┐  │
│  │  Client  │ ─────────────► │       Server         │  │
│  │          │ ◄───────────── │                      │  │
│  │  REPL    │  pipe (Result)  │  SQLParser           │  │
│  │          │                 │  SQLServer (executor)│  │
│  └──────────┘                 │  Table C++ wrapper   │  │
│                               │  Table.c (C driver)  │  │
│                               └──────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

Клиент и сервер работают как **отдельные процессы** на одной машине,
взаимодействуя через **анонимные pipes** с framing-протоколом
(4-байтовая длина + payload).

## Иерархия классов

```
std::runtime_error
 ├── ParseException          — ошибки разбора SQL
 ├── SQLExecutionException   — ошибки выполнения запроса
 ├── ProtocolException       — ошибки IPC-протокола
 └── TableException          — ошибки работы с таблицей
      ├── TableNotFoundException
      ├── TableFieldException
      └── TableIOException

Table          — RAII-обёртка над THandle (C-драйвер)
SQLParser      — токенизатор + рекурсивный парсер SQL
 └── Tokenizer — лексический анализатор
SQLServer      — движок выполнения запросов
CondEvaluator  — вычисление WHERE-условий на строке
ResultFormatter — сериализация/десериализация/pretty-print
SQLClient      — fork + exec сервера, отправка запросов

AST-узлы (value types):
  Token, CondExpr, Comparison, SelectStmt, InsertStmt,
  UpdateStmt, DeleteStmt, CreateTableStmt, DropTableStmt,
  ColDefAST, Assignment, SelectCols, LiteralValue (variant)

Data types:
  Row       — упорядоченный список (column → Value)
  Value     — variant<long, string> с типом FieldType
  ResultSet — columns + rows + message + affected
```

## Сборка

```bash
make          # собирает client и server
make clean    # удаляет артефакты
make run      # собирает и запускает клиент
```

## Поддерживаемый SQL

```sql
CREATE TABLE t (col1 LONG, col2 TEXT(32))
DROP TABLE t

INSERT INTO t (col1, col2) VALUES (42, 'hello')

SELECT * FROM t
SELECT col1, col2 FROM t WHERE col1 > 10

UPDATE t SET col2 = 'world' WHERE col1 = 42

DELETE FROM t WHERE col1 < 5
```

### WHERE-условия
```
col = val         col <> val
col < val         col > val
col <= val        col >= val
cond AND cond     cond OR cond
NOT cond          (cond)
```

## Пример сессии

```
sql> CREATE TABLE employees (id LONG, name TEXT(32), salary LONG);
Table 'employees' created
sql> INSERT INTO employees (id, name, salary) VALUES (1, 'Alice', 90000);
1 row inserted
sql> SELECT * FROM employees WHERE salary >= 80000;
+----+-------+--------+
| id | name  | salary |
+----+-------+--------+
| 1  | Alice | 90000  |
+----+-------+--------+
1 row(s) selected
sql> \quit
Goodbye.
```

## Файлы

| Файл               | Роль                                              |
|--------------------|---------------------------------------------------|
| `Table.c`          | Низкоуровневый C-драйвер таблицы (дано)           |
| `_Table.h`         | Заголовок C-драйвера с extern "C"                 |
| `Table.hpp`        | C++ RAII-обёртка над THandle                      |
| `SQLParser.hpp`    | Лексер + LL(1)-парсер SQL → AST                   |
| `SQLServer.hpp`    | Движок исполнения + вычислитель условий           |
| `Protocol.hpp`     | Framing-протокол для pipe IPC                     |
| `ResultFormatter.hpp` | Сериализация ResultSet, ASCII-таблица          |
| `server.cpp`       | Серверный процесс (main)                          |
| `client.cpp`       | Клиентский процесс (main, fork/exec, REPL)        |
| `Makefile`         | Сборка проекта                                    |
