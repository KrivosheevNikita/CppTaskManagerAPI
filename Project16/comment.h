#ifndef COMMENT_H
#define COMMENT_H

#include "crow_all.h"
#include "database.h"
#include "auth.h"
namespace comment
{
    crow::response addComment(const crow::request& req, int task_id);
    crow::response getComments(const crow::request& req, int task_id);
    crow::response updateComment(const crow::request& req, int task_id, int comment_id);
    crow::response deleteComment(const crow::request& req, int task_id, int comment_id);
}

#endif 