#include "database.h"

ConnectionPool::ConnectionPool(const std::string& connStr, unsigned int minSize, unsigned int maxSize)
    : connStr(connStr), minSize(minSize), maxSize(maxSize), curSize(0) 
{
    for (unsigned int i = 0; i != minSize; ++i) 
    {
        connections.push_back(std::make_shared<pqxx::connection>(connStr));
        ++curSize;
    }
}

// �������� ������������� ���������� ���� ����������
ConnectionPool& ConnectionPool::getInstance()
{
    static ConnectionPool pool(CONNECTION_STRING, MIN_SIZE_POOL, MAX_SIZE_POOL);
    return pool;
}

// ����� ���������� �� ����
std::shared_ptr<pqxx::connection> ConnectionPool::getConnection() 
{
    std::unique_lock<std::mutex> lock(mtx);

    if (connections.empty() && curSize < maxSize) 
    {
        connections.push_back(std::make_shared<pqxx::connection>(connStr));
        ++curSize;
    }

    poolWaiting.wait(lock, [this] { return !connections.empty(); });

    auto conn = connections.back();
    connections.pop_back();

    // ���� ���������� ���������, �� ������� ����� ����������
    if (!conn->is_open()) 
        conn = std::make_shared<pqxx::connection>(connStr);

    return conn;
}

// ������� ���������� � ��� 
void ConnectionPool::returnConnection(std::shared_ptr<pqxx::connection> conn) 
{
    std::unique_lock<std::mutex> lock(mtx);

    // ���������� ����������, ���� ��� �������
    if (conn->is_open())
        connections.push_back(conn);
    // ����� ��������� ������ ����
    else
        --curSize; 

    poolWaiting.notify_one();
}

// ���������� ��������� ����������
unsigned int ConnectionPool::availableConnections() const 
{
    return connections.size();
}

// ���������� �������� ����������
unsigned int ConnectionPool::activeConnections() const 
{
    return curSize - connections.size();
}

ConnectionGuard::ConnectionGuard() : conn(ConnectionPool::getInstance().getConnection()) {}

ConnectionGuard::~ConnectionGuard()
{
    ConnectionPool::getInstance().returnConnection(conn);
}

ConnectionGuard::operator pqxx::connection& ()
{
    return *conn;
}

bool ConnectionGuard::is_open() const
{
    return conn && conn->is_open();
}

// ���������� � ��
ConnectionGuard connectDB()
{
    return ConnectionGuard();
}

