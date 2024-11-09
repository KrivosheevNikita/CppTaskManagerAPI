#include "comment.h"

namespace comment
{
    crow::response addComment(const crow::request& req, int task_id)
    {
        try
        {
            int user_id;
            if (!auth::checkToken(req, user_id))
                return crow::response(401, "Missing or invalid authorization token");

            auto db = connectDB();
            if (!db.is_open())
                return crow::response(500, "Internal Server Error");

            auto json_data = crow::json::load(req.body);
            if (!json_data || !json_data.has("comment"))
                return crow::response(400, "Invalid or missing JSON data");

            std::string comment = json_data["comment"].s();

            pqxx::work txn(db);

            auto taskCheck = txn.exec_params(
                "SELECT 1 FROM tasks WHERE task_id = $1",
                task_id);

            if (taskCheck.empty())
                return crow::response(404, "Task not found");

            auto commentInsert = txn.exec_params(
                "INSERT INTO comments (task_id, user_id, comment) VALUES ($1, $2, $3) RETURNING comment_id",
                task_id, user_id, comment);

            int comment_id = commentInsert[0][0].as<int>();

            txn.commit();

            crow::json::wvalue response;
            response["message"] = "Comment added successfully";
            response["comment_id"] = comment_id; 

            return crow::response(201, response);
        }
        catch (const std::exception& e)
        {
            return crow::response(400, e.what());
        }
    }

    crow::response getComments(const crow::request& req, int task_id)
    {
        try
        {
            int user_id;
            if (!auth::checkToken(req, user_id))
                return crow::response(401, "Missing or invalid authorization token");

            auto db = connectDB();
            if (!db.is_open())
                return crow::response(500, "Internal Server Error");

            pqxx::nontransaction txn(db);
            auto result = txn.exec_params(
                "SELECT comment_id, comment, created_at, updated_at FROM comments WHERE task_id = $1 ORDER BY created_at ASC",
                task_id);

            crow::json::wvalue response;
            response["comments"] = crow::json::wvalue::list();
            int index = 0;

            for (const auto& row : result)
            {
                crow::json::wvalue comment;
                comment["comment_id"] = row["comment_id"].as<int>();
                comment["comment"] = row["comment"].as<std::string>();
                comment["created_at"] = row["created_at"].as<std::string>();
                comment["updated_at"] = row["updated_at"].as<std::string>();
                response["comments"][index++] = std::move(comment);
            }

            return crow::response(response);
        }
        catch (const std::exception& e)
        {
            return crow::response(400, e.what());
        }
    }

    crow::response updateComment(const crow::request& req, int task_id, int comment_id)
    {
        try
        {
            int user_id;
            if (!auth::checkToken(req, user_id))
                return crow::response(401, "Missing or invalid authorization token");

            auto db = connectDB();
            if (!db.is_open())
                return crow::response(500, "Internal Server Error");

            auto json_data = crow::json::load(req.body);
            if (!json_data || !json_data.has("comment"))
                return crow::response(400, "Invalid or missing JSON data");

            std::string comment = json_data["comment"].s();

            pqxx::nontransaction txn(db);

            auto result = txn.exec_params(
                "UPDATE comments SET comment = $1 WHERE comment_id = $2 AND task_id = $3 AND user_id = $4",
                comment, comment_id, task_id, user_id);

            if (result.affected_rows() == 0)
                return crow::response(403, "Access denied");

            return crow::response(200, "Comment updated successfully");
        }
        catch (const std::exception& e)
        {
            return crow::response(400, e.what());
        }
    }

    crow::response deleteComment(const crow::request& req, int task_id, int comment_id)
    {
        try
        {
            int user_id;
            if (!auth::checkToken(req, user_id))
                return crow::response(401, "Missing or invalid authorization token");

            auto db = connectDB();
            if (!db.is_open())
                return crow::response(500, "Internal Server Error");

            pqxx::nontransaction txn(db);
            auto result = txn.exec_params(
                "DELETE FROM comments WHERE comment_id = $1 AND task_id = $2 AND user_id = $3",
                comment_id, task_id, user_id
            );

            if (result.affected_rows() == 0)
                return crow::response(403, "Access denied");

            return crow::response(200, "Comment deleted successfully");
        }
        catch (const std::exception& e)
        {
            return crow::response(400, e.what());
        }
    }
}
