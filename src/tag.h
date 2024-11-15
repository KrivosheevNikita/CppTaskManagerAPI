#ifndef TAG_H
#define TAG_H

#include "crow_all.h"
#include <pqxx/pqxx>
#include <hiredis/hiredis.h>
#include "auth.h"
#include "cache.h"

namespace tag 
{
    std::vector<std::string> addTagsToTask(pqxx::work& txn, int task_id, const crow::json::rvalue& jsonData);
    void addTagsToResponse(std::string& tags, crow::json::wvalue& response);

    crow::response addTags(const crow::request& req, int task_id);
}

#endif 