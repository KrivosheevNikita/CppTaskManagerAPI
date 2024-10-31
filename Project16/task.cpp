#include "task.h"

namespace task
{
    
    crow::response createTask(const crow::request& req)
    {
        try
        {
            int user_id;
            // ��������� ����� � �������� user_id
            if (!auth::checkToken(req, user_id)) {
                return crow::response(401, "Missing or invalid authorization token");
            }

            auto db = connectDB();
            if (!db.is_open())
                return crow::response(500, "Internal server error");

            auto jsonData = crow::json::load(req.body);
            if (!jsonData || !jsonData.has("task_name") || !jsonData.has("description"))
                return crow::response(400, "Invalid or missing JSON");

            std::string task_name = jsonData["task_name"].s();
            std::string description = jsonData["description"].s();
            int status_id = jsonData.has("status_id") ? jsonData["status_id"].i() : 1; // �������� �� ��������� 1
            int priority = jsonData.has("priority") ? jsonData["priority"].i() : 1; // �������� �� ��������� 1
            std::string due_date = jsonData.has("due_date") ? jsonData["due_date"].s() : std::string("2099-12-31");

            pqxx::work txn(db);

            auto taskInsert = txn.exec_params(
                R"(INSERT INTO tasks (user_id, task_name, description, status_id, priority, due_date) 
                    VALUES ($1, $2, $3, $4, $5, $6) RETURNING task_id)",
                user_id, task_name, description, status_id, priority, due_date
            );

            int task_id = taskInsert[0][0].as<int>();

            // ��������� ������� ����� � ��������� ��, ���� ��� ����
            tag::addTagsToTask(txn, task_id, jsonData);

            txn.commit();

            crow::json::wvalue response;
            response["message"] = "Task was created successfully";
            response["task_id"] = task_id;
            return crow::response(200, response);
        }
        catch (const std::exception& e)
        {
            return crow::response(400, e.what());
        }
    }

    crow::response updateTask(const crow::request& req, int task_id)
    {
        try
        {
            int user_id;
            if (!auth::checkToken(req, user_id))
                return crow::response(401, "Missing or invalid authorization token");

            auto db = connectDB();
            if (!db.is_open())
                return crow::response(500, "Internal server error");

            auto jsonData = crow::json::load(req.body);
            if (!jsonData || !jsonData.has("task_name") || !jsonData.has("description"))
                return crow::response(400, "Missing or invalid JSON");

            std::string task_name = jsonData["task_name"].s();
            std::string description = jsonData["description"].s();
            int status_id = jsonData.has("status_id") ? jsonData["status_id"].i() : 1;
            int priority = jsonData.has("priority") ? jsonData["priority"].i() : 1;
            std::string due_date = jsonData.has("due_date") ? jsonData["due_date"].s() : std::string("2099-12-31");

            pqxx::work txn(db);

            auto result = txn.exec_params("SELECT task_id FROM tasks WHERE task_id = $1 AND user_id = $2", task_id, user_id);
            if (result.empty())
                return crow::response(403, "Access denied");

            // ���������� ���� ����� ������
            txn.exec_params("UPDATE tasks SET task_name = $1, description = $2, status_id = $3, priority = $4, due_date = $5 WHERE task_id = $6",
                task_name, description, status_id, priority, due_date, task_id);

            txn.exec_params("DELETE FROM task_tags WHERE task_id = $1", task_id);

            // ���������� �����
            tag::addTagsToTask(txn, task_id, jsonData);

            txn.commit();
            return crow::response(200, "Task updated successfully");
        }
        catch (const std::exception& e)
        {
            return crow::response(400, e.what());
        }
    }

    crow::response getTask(const crow::request& req, int task_id)
    {
        try
        {
            int user_id;
            if (!auth::checkToken(req, user_id))
            {
                return crow::response(401, "Missing or invalid authorization token");
            }
            auto db = connectDB();
            if (!db.is_open())
                return crow::response(500, "Internal server error");

            pqxx::nontransaction txn(db);

            auto result = txn.exec_params(
                R"(SELECT t.task_id, t.task_name, t.description, t.priority, t.due_date,
                s.status_name, COALESCE(array_agg(tag.tag_name), '{}') AS tags
                FROM tasks t
                LEFT JOIN task_tags tt ON t.task_id = tt.task_id
                LEFT JOIN tags tag ON tt.tag_id = tag.tag_id
                LEFT JOIN task_statuses s ON t.status_id = s.status_id
                WHERE t.task_id = $1 AND t.user_id = $2
                GROUP BY t.task_id, s.status_name)",
                task_id, user_id
            );

            if (result.empty())
            {
                return crow::response(403, "Access denied");
            }

            crow::json::wvalue response;
            response["task_id"] = result[0]["task_id"].as<int>();
            response["task_name"] = result[0]["task_name"].as<std::string>();
            response["description"] = result[0]["description"].as<std::string>();
            response["status"] = result[0]["status_name"].as<std::string>();
            response["priority"] = result[0]["priority"].as<int>();
            response["due_date"] = result[0]["due_date"].as<std::string>();

            auto tags = result[0]["tags"].as<std::string>();
            tag::addTagsToResponse(tags, response);

            return crow::response(response);
        }
        catch (const std::exception& e)
        {
            return crow::response(400, e.what());
        }
    }

    crow::response deleteTask(const crow::request& req, int task_id)
    {
        try
        {
            int user_id;
            if (!auth::checkToken(req, user_id)) {
                return crow::response(401, "Missing or invalid authorization token");
            }

            auto db = connectDB();
            if (!db.is_open())
                return crow::response(500, "Internal server error");

            pqxx::work txn(db);

            auto result = txn.exec_params(
                "SELECT task_id FROM tasks WHERE task_id = $1 AND user_id = $2", task_id, user_id
            );

            if (result.empty()) {
                return crow::response(404, "Task not found");
            }

            txn.exec_params("DELETE FROM task_tags WHERE task_id = $1", task_id);
            txn.exec_params("DELETE FROM tasks WHERE task_id = $1", task_id);

            txn.commit();

            return crow::response(200, "Task deleted successfully");
        }
        catch (const std::exception& e)
        {
            return crow::response(400, e.what());
        }
    }

    crow::response getAllTasks(const crow::request& req)
    {
        try
        {
            int user_id;
            if (!auth::checkToken(req, user_id))
                return crow::response(401, "Missing or invalid authorization token");

            auto db = connectDB();
            if (!db.is_open())
                return crow::response(500, "Internal Server Error");

            pqxx::work txn(db);

            auto result = txn.exec_params(
                R"(SELECT t.task_id, t.task_name, t.description, t.priority, t.due_date, 
                s.status_name, COALESCE(array_agg(tag.tag_name), '{}') AS tags
                FROM tasks t
                LEFT JOIN task_tags tt ON t.task_id = tt.task_id
                LEFT JOIN tags tag ON tt.tag_id = tag.tag_id
                LEFT JOIN task_statuses s ON t.status_id = s.status_id
                WHERE t.user_id = $1
                GROUP BY t.task_id, s.status_id, s.status_name
                ORDER BY s.status_id ASC, t.priority ASC, t.due_date ASC, t.task_id ASC)",
                user_id
            );

            crow::json::wvalue response;
            response["tasks"] = crow::json::wvalue::list();

            int index = 0;
            for (const auto& row : result)
            {
                crow::json::wvalue task;
                task["task_id"] = row["task_id"].as<int>();
                task["task_name"] = row["task_name"].as<std::string>();
                task["description"] = row["description"].as<std::string>();
                task["status"] = row["status_name"].as<std::string>();
                task["priority"] = row["priority"].as<int>();
                task["due_date"] = row["due_date"].as<std::string>();

                auto tags = row["tags"].as<std::string>();
                tag::addTagsToResponse(tags, task);

                response["tasks"][index++] = std::move(task);
            }

            return crow::response(response);
        }
        catch (const std::exception& e)
        {
            return crow::response(400, e.what());
        }
    }
}