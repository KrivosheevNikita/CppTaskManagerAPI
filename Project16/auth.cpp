#include "auth.h"

pqxx::connection connectDB()
{
    const std::string connStr = "dbname=taskManager user=postgres password=1234 host=localhost port=5432";
    return pqxx::connection(connStr);
}

namespace auth
{
    // Проверка токена и помещение id пользователя в переменную user_id
    bool checkToken(const crow::request& req, int& user_id)
    {
        try
        {
            std::string token = req.get_header_value("Authorization");
            if (token.empty() || !token.starts_with("Bearer "))
                return false;

            token = token.substr(7); // Удаление "Bearer " из токена
            auto decoded = jwt::decode(token);
            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{ "key" })
                .with_issuer("taskManager");
            verifier.verify(decoded);

            user_id = -1;

            for (auto& field : decoded.get_payload_json())
            {
                if (field.first == "user_id")
                {
                    user_id = std::stoi(field.second.to_str());
                }
            }

            if (user_id == -1)
            {
                return false;
            }

            return true;
        }
        catch (const std::exception& e) {}
    }

    // Генерация соли для хеширования
    std::string generateSalt(unsigned int length = 10)
    {
        std::random_device r;
        std::mt19937 gen(r());
        const std::string chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        std::uniform_int_distribution<> dis(0, chars.size() - 1);

        std::stringstream ss;
        for (int i = 0; i != length; ++i) {
            ss << chars[dis(gen)];
        }
        return ss.str();
    }

    // Хеширование пароля 
    std::string hashPassword(const std::string& password, std::string& salt)
    {
        if (salt.empty())
            salt = generateSalt();

        char hash[16];
        char encoded[128];

        const unsigned int t_cost = 1;
        const unsigned int m_cost = (1 << 10);
        const unsigned int parallelism = 2;

        int result = argon2id_hash_encoded(t_cost, m_cost, parallelism,
            password.c_str(), password.length(),
            salt.c_str(), salt.length(),
            sizeof(hash), encoded, sizeof(encoded));

        if (result != ARGON2_OK)
        {
            throw std::runtime_error("Failed to hash password");
        }

        return std::string(encoded);
    }

    // Проверка пароля
    bool verifyPassword(const std::string& password, std::string& salt, const std::string& hash)
    {
        std::string hashedPassword = hashPassword(password, salt);
        return hashedPassword == hash;
    }

    // Проверка корректности данных пользователя и помещение id пользователя в переменную user_id
    bool checkUser(const std::string& username, const std::string& password, int& user_id)
    {
        auto db = connectDB();
        pqxx::nontransaction txn(db);
        pqxx::result result = txn.exec_params(
            "SELECT user_id, password, salt FROM users WHERE username = $1", username);

        if (result.empty())
            return false;

        std::string salt = result[0]["salt"].c_str();
        std::string hashedPassword = result[0]["password"].c_str();

        if (!verifyPassword(password, salt, hashedPassword))
            return false;

        user_id = result[0]["user_id"].as<int>();
        return true;
    }

    // Генерация JWT токена
    std::string generateToken(int user_id) {
        return jwt::create()
            .set_issuer("taskManager")
            .set_type("JWT")
            .set_payload_claim("user_id", jwt::claim(std::to_string(user_id)))
            .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(1))
            .sign(jwt::algorithm::hs256{ "key" });
    }

    // Авторизация, возврат токена 
    crow::response login(const crow::request& req)
    {
        try
        {
            auto jsonData = crow::json::load(req.body);
            if (!jsonData || !jsonData.has("username") || !jsonData.has("password"))
            {
                return crow::response(400, "Missing or invalid JSON");
            }

            std::string username = jsonData["username"].s();
            std::string password = jsonData["password"].s();
            int user_id;
            if (!checkUser(username, password, user_id))
            {
                return crow::response(401, "Invalid username or password");
            }

            auto token = generateToken(user_id);

            crow::json::wvalue response;
            response["status"] = "success";
            response["token"] = token;
            return crow::response(200, response);
        }
        catch (const std::exception& e)
        {
            return crow::response(500, "Internal server error");
        }
    }

    crow::response registerUser(const crow::request& req)
    {
        try
        {
            auto db = connectDB();

            auto jsonData = crow::json::load(req.body);
            if (!jsonData || !jsonData.has("username") || !jsonData.has("password") || !jsonData.has("email"))
            {
                return crow::response(400, "Missing or invalid JSON");
            }

            std::string username = jsonData["username"].s();
            std::string password = jsonData["password"].s();
            std::string email = jsonData["email"].s();

            pqxx::work txn(db);

            pqxx::result result = txn.exec_params(
                "SELECT user_id FROM users WHERE username = $1", username);
            pqxx::result resultEmail = txn.exec_params(
                "SELECT user_id FROM users WHERE email = $1", email);

            if (!result.empty())
            {
                return crow::response(409, "User already exists");
            }
            else if (!resultEmail.empty())
            {
                return crow::response(409, "This email is already in use");
            }

            std::string salt;
            std::string hashedPassword = hashPassword(password, salt);

            txn.exec_params(
                "INSERT INTO users (username, password, email, salt) VALUES ($1, $2, $3, $4)",
                username, hashedPassword, email, salt);
            txn.commit();

            return crow::response(200, "User registered successfully");
        }
        catch (const std::exception& e)
        {
            return crow::response(500, "Internal server error");
        }
    }
}
