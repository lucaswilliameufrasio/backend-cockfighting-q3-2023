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
    static const size_t digit_pos[8] = {0, 1, 2, 3, 5, 6, 8, 9};
    for (const size_t pos : digit_pos) {
        if (date[pos] < '0' || date[pos] > '9') {
            return false;
        }
    }
    const int y = (date[0] - '0') * 1000 + (date[1] - '0') * 100 +
                  (date[2] - '0') * 10 + (date[3] - '0');
    const int m = (date[5] - '0') * 10 + (date[6] - '0');
    const int d = (date[8] - '0') * 10 + (date[9] - '0');
    if (y < 1800 || y > 9999 || m < 1 || m > 12 || d < 1 || d > 31) {
        return false;
    }
    if (m == 2) {
        const bool leap = ((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0);
        return leap ? d <= 29 : d <= 28;
    }
    if (m == 4 || m == 6 || m == 9 || m == 11) {
        return d <= 30;
    }
    return true;
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
