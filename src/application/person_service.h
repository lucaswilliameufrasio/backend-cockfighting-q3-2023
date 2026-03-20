#pragma once
#include "../infrastructure/person_repository.h"
#include <unordered_map>
#include <shared_mutex>
#include <memory>

namespace application {

// Lazy-initialized cache to save memory in constrained environments
static std::unique_ptr<std::unordered_map<std::string, domain::Person>> person_cache;
static std::unique_ptr<std::shared_mutex> cache_mutex;

struct PersonService {
    static void initCache(bool enable) {
        if (enable && !person_cache) {
            person_cache = std::make_unique<std::unordered_map<std::string, domain::Person>>();
            cache_mutex = std::make_unique<std::shared_mutex>();
        }
    }

    static drogon::Task<std::optional<domain::Person>> getPersonById(const std::string &id, bool useCache) {
        if (useCache && person_cache && cache_mutex) {
            std::shared_lock lock(*cache_mutex);
            auto it = person_cache->find(id);
            if (it != person_cache->end()) {
                co_return it->second;
            }
        }

        auto person = co_await infrastructure::PersonRepository::findById(id);
        
        if (useCache && person && person_cache && cache_mutex) {
            std::unique_lock lock(*cache_mutex);
            (*person_cache)[id] = *person;
        }
        co_return person;
    }

    static drogon::Task<std::vector<domain::Person>> searchPeople(const std::string &term) {
        co_return co_await infrastructure::PersonRepository::search(term);
    }

    static drogon::Task<size_t> getPersonCount() {
        co_return co_await infrastructure::PersonRepository::count();
    }

    static drogon::Task<std::optional<std::string>> createPerson(const domain::Person &person) {
        co_return co_await infrastructure::PersonRepository::save(person);
    }
};

} // namespace application
