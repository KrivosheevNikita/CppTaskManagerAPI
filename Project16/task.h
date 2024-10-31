#ifndef TASK_H
#define TASK_H

#include "crow_all.h"
#include <pqxx/pqxx>
#include <string>
#include "auth.h"
#include "tag.h"

namespace task
{
	crow::response createTask(const crow::request& req);
	crow::response updateTask(const crow::request& req, int task_id);
	crow::response getTask(const crow::request& req, int task_id);
	crow::response getAllTasks(const crow::request& req);
	crow::response deleteTask(const crow::request& req, int task_id);
}
#endif 