#include "auth.h"
#include "task.h"
#include "tag.h"
#include "database.h"
int main() 
{
    ConnectionPool::getInstance(); // Создание пула соединений к БД

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

    app.port(8080).multithreaded().run();
    return 0;
}
