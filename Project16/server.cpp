#include "auth.h"
#include "task.h"
#include "tag.h"
#include "database.h"
#include "cache.h"
#include "comment.h"
int main() 
{
    ConnectionPool::getInstance(); // Создание пула соединений к БД
    RedisConnectionPool::getInstance(); // Создание пула соединений к Redis
    
    crow::SimpleApp app; 

    // Регистрация
    CROW_ROUTE(app, "/register").methods("POST"_method)([](const crow::request& req) 
    {
        return auth::registerUser(req); 
    });

    // Авторизация
    CROW_ROUTE(app, "/login").methods("POST"_method)([](const crow::request& req) 
    {
        return auth::login(req); 
    });

    // Создание новой задачи
    CROW_ROUTE(app, "/tasks").methods("POST"_method)([](const crow::request& req)
    {
        return task::createTask(req);
    });

    // Получение задачи по id 
    CROW_ROUTE(app, "/tasks/<int>").methods("GET"_method)([](const crow::request& req, int task_id)
    {
            return task::getTask(req, task_id);
    });

    // Обновление задачи по id 
    CROW_ROUTE(app, "/tasks/<int>").methods("PUT"_method)([](const crow::request& req, int task_id)
    {
        return task::updateTask(req, task_id);
    });

    // Удаление задачи по id
    CROW_ROUTE(app, "/tasks/<int>").methods("DELETE"_method)([](const crow::request& req, int task_id)
    {
            return task::deleteTask(req, task_id);
    });

    // Получение списка всех задач пользователя
    CROW_ROUTE(app, "/tasks").methods("GET"_method)([](const crow::request& req)
    {
            return task::getAllTasks(req);
    });

    // Добавление тегов к задаче
    CROW_ROUTE(app, "/tasks/<int>/tags").methods("POST"_method)([](const crow::request& req, int task_id)
    {
        return tag::addTags(req, task_id);
    });

    // Добавление комментария к задаче
    CROW_ROUTE(app, "/tasks/<int>/comments").methods("POST"_method)([](const crow::request& req, int task_id) 
    {
        return comment::addComment(req, task_id);
    });

    // Получение всех комментариев к задаче
    CROW_ROUTE(app, "/tasks/<int>/comments").methods("GET"_method)([](const crow::request& req, int task_id) 
    {
        return comment::getComments(req, task_id);
    });

    // Обновление комментария по id
    CROW_ROUTE(app, "/tasks/<int>/comments/<int>").methods("PUT"_method)([](const crow::request& req, int task_id, int comment_id) 
    {
        return comment::updateComment(req, task_id, comment_id);
    });

    // Удаление комментария по id
    CROW_ROUTE(app, "/tasks/<int>/comments/<int>").methods("DELETE"_method)([](const crow::request& req, int task_id, int comment_id) 
    {
        return comment::deleteComment(req, task_id, comment_id);
    });

    app.port(8080).multithreaded().run();
    return 0;
}
