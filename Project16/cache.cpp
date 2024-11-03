#include "cache.h"

RedisConnectionPool::RedisConnectionPool(const std::string& host, int port, unsigned int minSize, unsigned int maxSize)
    : host(host), port(port), minSize(minSize), maxSize(maxSize), curSize(0)
{
    for (unsigned int i = 0; i != minSize; ++i)
    {
        connections.push_back(createConnection());
        ++curSize;
    }
}

// ��������� ������������� ���������� ���� ����������
RedisConnectionPool& RedisConnectionPool::getInstance()
{
    static RedisConnectionPool pool(HOST, PORT, MIN_SIZE_R, MAX_SIZE_R);
    return pool;
}

// ����� ���������� �� ����
redisContext* RedisConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> lock(mtx);

    if (connections.empty() && curSize < maxSize)
    {
        connections.push_back(createConnection());
        ++curSize;
    }

    poolWaiting.wait(lock, [this] { return !connections.empty(); });

    auto conn = connections.back();
    connections.pop_back();

    if (!conn || conn->err)
    {
        conn = createConnection();
    }

    return conn;
}

// ������� ���������� � ��� 
void RedisConnectionPool::returnConnection(redisContext* conn)
{
    std::unique_lock<std::mutex> lock(mtx);

    connections.push_back(conn);

    poolWaiting.notify_one();
}

// �������� ������ ����������
redisContext* RedisConnectionPool::createConnection()
{
    timeval timeout = { 0, 0 }; // ���� ���������� �� ���������������, �� �� ���� 
    return redisConnectWithTimeout(host.c_str(), port, timeout);
}

// ���������� ��������� ����������
unsigned int RedisConnectionPool::availableConnections() const
{
    return connections.size();
}

// ���������� �������� ����������
unsigned int RedisConnectionPool::activeConnections() const
{
    return curSize - connections.size();
}

RedisConnectionGuard::RedisConnectionGuard()
    : conn(RedisConnectionPool::getInstance().getConnection()) {}

RedisConnectionGuard::~RedisConnectionGuard()
{
    RedisConnectionPool::getInstance().returnConnection(conn);
}

// ����������� � redis
RedisConnectionGuard connectRedis()
{
    return RedisConnectionGuard();
}

// ������������ �����
inline std::string createCacheKey(int task_id, int user_id) {
    return "task:" + std::to_string(task_id) + ":user:" + std::to_string(user_id);
}

// ��������� ������ �� ����
crow::json::wvalue getTaskFromCache(int task_id, int user_id)
{
    crow::json::wvalue response;
    auto redis = connectRedis();
    if (redis)
    {
        std::string cacheKey = createCacheKey(task_id, user_id);
        redisReply* reply = (redisReply*)redisCommand(redis, "GET %s", cacheKey.c_str());
        if (reply && reply->type == REDIS_REPLY_STRING)
        {
            response = crow::json::load(reply->str);
            redisCommand(redis, "EXPIRE %s 300", cacheKey.c_str()); // ���������� ����� ����� ����� �� 5 �����
            freeReplyObject(reply);
        }
        else
        {
            response["null"] = "null"; // ����������� �����, ������������, ��� ����� � ���� �� ������
        }
    }
    return response;
}

// ���������� ������ � ����
void saveTaskInCache(int task_id, int user_id, const crow::json::wvalue& response)
{
    auto redis = connectRedis();
    if (redis)
    {
        std::string cacheKey = createCacheKey(task_id, user_id);
        std::string jsonData = response.dump();
        redisCommand(redis, "SETEX %s 300 %s", cacheKey.c_str(), jsonData.c_str()); // ������������� ����� ����� ����� 5 �����
    }
}

void saveTaskInCache(int task_id, int user_id, std::string task_name, std::string description
    , std::string status_name, int priority, std::string due_date, std::vector<std::string> tags)
{
    crow::json::wvalue jsonForCache;
    jsonForCache["task_id"] = task_id;
    jsonForCache["task_name"] = task_name;
    jsonForCache["description"] = description;
    jsonForCache["status_name"] = status_name;
    jsonForCache["priority"] = priority;
    jsonForCache["due_date"] = due_date;

    if (tags.size() != 0)
    {
        for (int i = 0; i != tags.size(); ++i)
        {
            jsonForCache["tags"][i] = tags[i];
        }
    }
    else
        jsonForCache["tags"][0];

    saveTaskInCache(task_id, user_id, jsonForCache);
}

void deleteTaskFromCache(int task_id, int user_id)
{
    auto redis = connectRedis();
    if (redis)
    {
        std::string cacheKey = createCacheKey(task_id, user_id);
        redisCommand(redis, "DEL %s", cacheKey.c_str());
    }
}
