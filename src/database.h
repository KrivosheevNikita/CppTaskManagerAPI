#ifndef DATABASE_H
#define DATABASE_H

#include <pqxx/pqxx>
#include <mutex>
#include <condition_variable>

const std::string CONNECTION_STRING = "dbname=taskManager user=postgres password=1234 host=db port=5432";
const int MIN_SIZE_POOL = 3;
const int MAX_SIZE_POOL = 10;

class ConnectionPool 
{
public:
    static ConnectionPool& getInstance();
    std::shared_ptr<pqxx::connection> getConnection();
    void returnConnection(std::shared_ptr<pqxx::connection> conn);
    unsigned int availableConnections() const;
    unsigned int activeConnections() const;

private:
    ConnectionPool(const std::string& connStr, unsigned int minSize, unsigned int maxSize);
    std::string connStr;
    unsigned int minSize;
    unsigned int maxSize;
    unsigned int curSize;
    std::mutex mtx;
    std::condition_variable poolWaiting;
    std::vector<std::shared_ptr<pqxx::connection>> connections;
};

// Класс для автоматического возврата соединения в пул после выхода из зоны видимости
class ConnectionGuard 
{
public:
    ConnectionGuard();
    ~ConnectionGuard();
    operator pqxx::connection& ();
    bool is_open() const;

private:
    std::shared_ptr<pqxx::connection> conn;
};

ConnectionGuard connectDB();

#endif 