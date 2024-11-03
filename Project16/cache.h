#ifndef CACHE_H
#define CACHE_H

#include <hiredis/hiredis.h>
#include "crow_all.h"
#include <mutex>
#include <vector>
#include <string>

const std::string HOST = "127.0.0.1";
const int PORT = 6379;
const int MIN_SIZE_R = 3;
const int MAX_SIZE_R = 10;

class RedisConnectionPool
{
public:
    static RedisConnectionPool& getInstance();
    redisContext* getConnection();
    void returnConnection(redisContext* conn);
    unsigned int availableConnections() const;
    unsigned int activeConnections() const;

private:
    RedisConnectionPool(const std::string& host, int port, unsigned int minSize, unsigned int maxSize);
    std::string host;
    int port;
    unsigned int minSize;
    unsigned int maxSize;
    unsigned int curSize;
    std::mutex mtx;
    std::condition_variable poolWaiting;
    std::vector<redisContext*> connections;

    redisContext* createConnection();
};

// Класс для автоматического возврата соединения в пул после выхода из зоны видимости
class RedisConnectionGuard
{
public:
    RedisConnectionGuard();
    ~RedisConnectionGuard();
    operator redisContext* () { return conn; }

private:
    redisContext* conn;
};

RedisConnectionGuard connectRedis();

std::string createCacheKey(int task_id, int user_id);
crow::json::wvalue getTaskFromCache(int task_id, int user_id);
void saveTaskInCache(int task_id, int user_id, const crow::json::wvalue& response);
void saveTaskInCache(int task_id, int user_id, std::string task_name, std::string description
    , std::string status_name, int priority, std::string due_date, std::vector<std::string> tags);
void deleteTaskFromCache(int task_id, int user_id);

#endif 
