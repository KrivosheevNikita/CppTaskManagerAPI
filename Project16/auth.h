#ifndef AUTH_H
#define AUTH_H

#include "crow_all.h"
#include <pqxx/pqxx>
#include <jwt-cpp/jwt.h>
#include "argon2.h"
#include <string>

pqxx::connection connectDB();

namespace auth 
{

	bool checkToken(const crow::request& req, int& user_id);
	bool checkUser(const std::string& username, const std::string& password, int& user_id);
	bool verifyPassword(const std::string& password, std::string& salt, const std::string& hash);

	std::string generateToken(int user_id);

	crow::response login(const crow::request& req);
	crow::response registerUser(const crow::request& req);

	std::string generateSalt(unsigned int length);
	std::string hashPassword(const std::string& password, std::string& salt);
}

#endif 
