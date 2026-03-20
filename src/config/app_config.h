#pragma once
#include <drogon/drogon.h>
#include <sstream>
#include <vector>

namespace config {

inline bool isDateValid(const std::string &date) {
    if (date.size() != 10) {
        return false;
    }
    if (date[4] != '-' || date[7] != '-') {
        return false;
    }
    
    try {
        int y = std::stoi(date.substr(0, 4));
        int m = std::stoi(date.substr(5, 2));
        int d = std::stoi(date.substr(8, 2));
        if (y < 1800 || y > 9999 || m < 1 || m > 12 || d < 1 || d > 31) {
            return false;
        }
        if (m == 2) {
            bool leap = ((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0);
            return leap ? d <= 29 : d <= 28;
        }
        if (m == 4 || m == 6 || m == 9 || m == 11) {
            return d <= 30;
        }
        return true;
    } catch (...) {
        return false;
    }
}

struct AppConfig {
    int port;
    size_t dbMaxConnections;
    int numThreads;
    std::string dbHost, dbName, dbUser, dbPassword;
    unsigned short dbPort;

    static AppConfig load() {
        return {
            std::stoi(getenv("PORT") ?: "8080"),
            std::stoul(getenv("DB_MAX_CONNECTIONS") ?: "40"),
            std::stoi(getenv("NUM_THREADS") ?: "2"),
            getenv("DB_HOST") ?: "localhost",
            getenv("DB_NAME") ?: "fight",
            getenv("DB_USER") ?: "postgres",
            getenv("DB_PASSWORD") ?: "fight",
            (unsigned short)std::stoi(getenv("DB_PORT") ?: "5432")
        };
    }
};

} // namespace config
