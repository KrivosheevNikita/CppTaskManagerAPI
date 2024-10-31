#include "auth.h"
#include "task.h"
#include "tag.h"

int main() 
{
    crow::SimpleApp app; 

    // �����������
    CROW_ROUTE(app, "/register").methods("POST"_method)([](const crow::request& req) 
    {
        return auth::registerUser(req); 
    });

    // �����������
    CROW_ROUTE(app, "/login").methods("POST"_method)([](const crow::request& req) 
    {
        return auth::login(req); 
    });

    // �������� ����� ������
    CROW_ROUTE(app, "/task").methods("POST"_method)([](const crow::request& req)
    {
        return task::createTask(req);
    });


    // ���������� ������
    CROW_ROUTE(app, "/task/<int>").methods("PUT"_method)([](const crow::request& req, int task_id)
    {
        return task::updateTask(req, task_id);
    });

    // �������� ������ �� task_id
    CROW_ROUTE(app, "/task/<int>").methods("DELETE"_method)([](const crow::request& req, int task_id)
        {
            return task::deleteTask(req, task_id);
        });

    // ��������� ������ �� id 
    CROW_ROUTE(app, "/task/<int>").methods("GET"_method)([](const crow::request& req, int task_id)
    {
        return task::getTask(req, task_id);
    });


    // ��������� ������ ���� ����� ������������
    CROW_ROUTE(app, "/tasks").methods("GET"_method)([](const crow::request& req)
    {
            return task::getAllTasks(req);
    });

    // ���������� ����� � ������
    CROW_ROUTE(app, "/task/<int>/tags").methods("POST"_method)([](const crow::request& req, int task_id)
    {
        return tag::addTags(req, task_id);
    });

    app.port(8080).multithreaded().run();
    return 0;
}
