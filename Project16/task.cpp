#include "task.h"

namespace task
{
    crow::response createTask(const crow::request& req)
    {
        try
        {
            int user_id;
            // Проверяем токен и получаем user_id
            if (!auth::checkToken(req, user_id)) 
                return crow::response(401, "Missing or invalid authorization token");

            auto db = connectDB();
            if (!db.is_open())
                return crow::response(500, "Internal server error");

            auto jsonData = crow::json::load(req.body);
            if (!jsonData || !jsonData.has("task_name") || !jsonData.has("description"))
                return crow::response(400, "Invalid or missing JSON");

            std::string task_name = jsonData["task_name"].s();
            std::string description = jsonData["description"].s();
            int status_id = jsonData.has("status_id") ? jsonData["status_id"].i() : 1; // Значение по умолчанию 1
            int priority = jsonData.has("priority") ? jsonData["priority"].i() : 1; // Значение по умолчанию 1
            std::string due_date = jsonData.has("due_date") ? jsonData["due_date"].s() : std::string("2099-12-31");

            pqxx::work txn(db);

            auto taskInsert = txn.exec_params(
                R"(WITH ins AS (
                INSERT INTO tasks (user_id, task_name, description, status_id, priority, due_date) 
                VALUES ($1, $2, $3, $4, $5, $6) RETURNING task_id, status_id)
                SELECT ins.task_id, s.status_name 
                FROM ins 
                JOIN task_statuses s ON ins.status_id = s.status_id)",
                user_id, task_name, description, status_id, priority, due_date
            );

            int task_id = taskInsert[0][0].as<int>();

            // Проверяем наличие тегов и добавляем их, если они есть
            std::vector<std::string> tags = tag::addTagsToTask(txn, task_id, jsonData);
            std::string status_name = taskInsert[0]["status_name"].as<std::string>();

            txn.commit();

            // Сохраняем в кэш
            saveTaskInCache(task_id, user_id, task_name, description, status_name, priority, due_date, tags);

            crow::json::wvalue response;
            response["message"] = "Task was created successfully";
            response["task_id"] = task_id; // Возвращаем id созданной задачи
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

            // Обновление всех полей задачи
            auto updateResult = txn.exec_params(
                R"(UPDATE tasks 
                SET task_name = $1, description = $2, status_id = $3, priority = $4, due_date = $5 
                WHERE task_id = $6 
                RETURNING (SELECT status_name FROM task_statuses WHERE status_id = $3) AS status_name)",
                task_name, description, status_id, priority, due_date, task_id
            );

            // Удаляем старые теги
            txn.exec_params("DELETE FROM task_tags WHERE task_id = $1", task_id);

            // Добавляем новые теги 
            tag::addTagsToTask(txn, task_id, jsonData);

            txn.commit();

            std::vector<std::string> tags = tag::addTagsToTask(txn, task_id, jsonData);
            std::string status_name = updateResult[0]["status_name"].as<std::string>();

            // Сохраняем обновленную задачу в кэш
            saveTaskInCache(task_id, user_id, task_name, description, status_name, priority, due_date, tags); 

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
                return crow::response(401, "Missing or invalid authorization token");

            auto responseCache = getTaskFromCache(task_id, user_id);

            if (!responseCache.count("null")) // Проверяем, нашелся ли ответ в кэше, используя метку "null"
                return responseCache;

            // Если задачи нет в кэше, идем в бд
            auto db = connectDB();
            if (!db.is_open())
            {
                return crow::response(500, "Internal server error");
            }

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

            // Сохраняем задачу в кэш
            saveTaskInCache(task_id, user_id, response);

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
            if (!auth::checkToken(req, user_id))
                return crow::response(401, "Missing or invalid authorization token");

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

            // Так же удаляем из кэша
            deleteTaskFromCache(task_id, user_id);

            return crow::response(200, "Task deleted successfully");
        }
        catch (const std::exception& e)
        {
            return crow::response(400, e.what());
        }
    }

    std::string buildQueryWithFilterTags(int user_id, const std::string& tagsFilter)
    {
        std::string query = R"(
            SELECT t.task_id, t.task_name, t.description, t.priority, t.due_date, 
            s.status_name, COALESCE(array_agg(tag.tag_name), '{}') AS tags
            FROM tasks t
            LEFT JOIN task_tags tt ON t.task_id = tt.task_id
            LEFT JOIN tags tag ON tt.tag_id = tag.tag_id
            LEFT JOIN task_statuses s ON t.status_id = s.status_id
            WHERE t.user_id = )" + std::to_string(user_id);

        // Если в параметрах есть теги, то добавляем условие для фильтрации
        if (!tagsFilter.empty())
        {
            std::vector<std::string> tags;
            std::stringstream tag_stream(tagsFilter);
            std::string tag;

            while (std::getline(tag_stream, tag, ','))
            {
                tags.push_back(tag);
            }

            query += R"(
                AND t.task_id IN (
                SELECT tt.task_id
                FROM task_tags tt
                JOIN tags tag ON tt.tag_id = tag.tag_id
                WHERE tag.tag_name = ANY('{)" + tagsFilter + R"(}')
                GROUP BY tt.task_id
                HAVING COUNT(DISTINCT tag.tag_name) = )" + std::to_string(tags.size()) + ")";
        }

        query += R"(
        GROUP BY t.task_id, s.status_id, s.status_name
        ORDER BY t.priority DESC, t.due_date ASC, t.task_id ASC)";

        return query;
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

            crow::query_string qs(req.url_params);
            std::string tagsFilter = qs.get("tags") ? qs.get("tags") : "";

            std::string query = buildQueryWithFilterTags(user_id, tagsFilter);

            pqxx::nontransaction txn(db);
            pqxx::result result = txn.exec(query);

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