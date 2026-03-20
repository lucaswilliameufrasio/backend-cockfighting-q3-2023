#pragma once
#include <drogon/drogon.h>
#include "../domain/person.h"
#include <optional>
#include <vector>

namespace infrastructure {

inline std::string to_pg_array(const std::vector<std::string> &v) {
    if (v.empty()) {
        return "{}";
    }
    std::string s = "{";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) {
            s += ",";
        }
        s += "\"" + v[i] + "\"";
    }
    s += "}";
    return s;
}

inline std::vector<std::string> from_pg_array(const std::string &s) {
    if (s.size() <= 2) {
        return {};
    }
    std::vector<std::string> v;
    std::string current;
    bool in_quotes = false;
    for (size_t i = 1; i < s.size() - 1; ++i) {
        if (s[i] == '"') {
            in_quotes = !in_quotes;
        } else if (s[i] == ',' && !in_quotes) {
            v.push_back(current);
            current.clear();
        } else {
            current += s[i];
        }
    }
    if (!current.empty()) {
        v.push_back(current);
    }
    return v;
}

struct PersonRepository {
    static drogon::Task<std::optional<domain::Person>> findById(const std::string &id) {
        auto clientPtr = drogon::app().getDbClient();
        try {
            auto result = co_await clientPtr->execSqlCoro("SELECT id, nickname, name, birth_date, stack FROM people WHERE id = $1;", id);
            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result[0];
            domain::Person p;
            p.id = row["id"].as<std::string>();
            p.nickname = row["nickname"].as<std::string>();
            p.name = row["name"].as<std::string>();
            p.birth_date = row["birth_date"].as<std::string>();
            if (!row["stack"].isNull()) {
                p.stack = from_pg_array(row["stack"].as<std::string>());
            }
            co_return p;
        } catch (...) {
            co_return std::nullopt;
        }
    }

    static drogon::Task<std::vector<domain::Person>> search(const std::string &term) {
        auto clientPtr = drogon::app().getDbClient();
        std::vector<domain::Person> people;
        try {
            // Optimization: Query the pre-calculated 'searchable' column instead of concatenating on the fly
            auto result = co_await clientPtr->execSqlCoro(
                "SELECT id, nickname, name, birth_date, stack FROM people "
                "WHERE searchable LIKE $1 "
                "LIMIT 50;",
                "%" + term + "%");

            for (const auto &row : result) {
                domain::Person p;
                p.id = row["id"].as<std::string>();
                p.nickname = row["nickname"].as<std::string>();
                p.name = row["name"].as<std::string>();
                p.birth_date = row["birth_date"].as<std::string>();
                if (!row["stack"].isNull()) {
                    p.stack = from_pg_array(row["stack"].as<std::string>());
                }
                people.push_back(std::move(p));
            }
        } catch (...) {
        }
        co_return people;
    }

    static drogon::Task<size_t> count() {
        auto clientPtr = drogon::app().getDbClient();
        try {
            auto result = co_await clientPtr->execSqlCoro("SELECT COUNT(*) AS count FROM people;");
            if (result.empty()) {
                co_return 0;
            }
            co_return result[0]["count"].as<size_t>();
        } catch (...) {
            co_return 0;
        }
    }

    static drogon::Task<std::optional<std::string>> save(const domain::Person &p) {
        auto clientPtr = drogon::app().getDbClient();
        try {
            auto result = co_await clientPtr->execSqlCoro(
                "INSERT INTO people (nickname, name, birth_date, stack) "
                "VALUES ($1, $2, $3, $4) "
                "ON CONFLICT (nickname) DO NOTHING "
                "RETURNING id;",
                p.nickname, p.name, p.birth_date, to_pg_array(p.stack.value_or(std::vector<std::string>{})));
            
            if (result.empty()) {
                co_return std::nullopt;
            }
            co_return result[0]["id"].as<std::string>();
        } catch (...) {
            co_return std::nullopt;
        }
    }
};

} // namespace infrastructure
