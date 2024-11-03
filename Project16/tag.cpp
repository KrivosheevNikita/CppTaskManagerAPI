#include "tag.h"

namespace tag 
{
    std::vector<std::string> addTagsToTask(pqxx::work& txn, int task_id, const crow::json::rvalue& jsonData) 
    {
        std::vector<std::string> tags;

        if (jsonData.has("tags")) {
            for (const auto& tag : jsonData["tags"]) {
                std::string tag_name = tag.s();
                tags.push_back(tag_name); // Добавляем тег в список для возвращения

                // Проверяем, существует ли тег, если нет, то добавляем его
                auto tagCheck = txn.exec_params("SELECT tag_id FROM tags WHERE tag_name = $1", tag_name);
                int tag_id;

                if (tagCheck.empty()) 
                {
                    // Если тег не существует, создаем новый
                    auto insertTag = txn.exec_params(
                        "INSERT INTO tags (tag_name) VALUES ($1) RETURNING tag_id", tag_name
                    );
                    tag_id = insertTag[0][0].as<int>();
                }
                else 
                {
                    tag_id = tagCheck[0][0].as<int>();
                }

                // Привязываем тег к задаче
                txn.exec_params("INSERT INTO task_tags (task_id, tag_id) VALUES ($1, $2) ON CONFLICT DO NOTHING", task_id, tag_id);
            }
        }

        return tags; // Возвращаем список тегов для использования в кэше
    }


    void addTagsToResponse(std::string& tags, crow::json::wvalue& response)
    {
        if (tags.empty() || tags == "{}" || tags == "{NULL}")
        {
            response["tags"] = crow::json::wvalue::list();
            return;
        }
        if (tags.front() == '{' && tags.back() == '}')
        {
            tags = tags.substr(1, tags.size() - 2);
        }

        std::istringstream ss(tags);
        std::string tag;

        int index = 0;
        response["tags"] = crow::json::wvalue::list();

        while (std::getline(ss, tag, ','))
        {
            response["tags"][index++] = tag;
        }
    }

    crow::response addTags(const crow::request& req, int task_id)
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
            if (!jsonData || !jsonData.has("tags")) {
                return crow::response(400, "Invalid or missing JSON");
            }

            pqxx::work txn(db);

            // Проверка, существует ли задача с task_id и принадлежит ли она пользователю
            auto taskCheck = txn.exec_params(
                "SELECT task_id FROM tasks WHERE task_id = $1 AND user_id = $2", task_id, user_id
            );

            if (taskCheck.empty())
                return crow::response(403, "Access denied");

            // Добавляем теги к задаче
            tag::addTagsToTask(txn, task_id, jsonData);

            txn.commit();

            // Удаляем из кэша, так как данные в кэше стали неактуальными
            deleteTaskFromCache(task_id, user_id);

            return crow::response(200, "Tags were added successfully");
        }
        catch (const std::exception& e)
        {
            return crow::response(400, e.what());
        }
    }
}