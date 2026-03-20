#include <drogon/drogon.h>
#include "config/app_config.h"
#include "config/resource_tuner.h"
#include "transport/http/person_handler.h"

using namespace drogon;
using namespace transport::http;

int main() {
    auto config = config::AppConfig::load();
    auto tuner = config::TuningProfile::detect(config.numThreads, config.dbMaxConnections);
    
    PersonHandler::setTuner(tuner);
    application::PersonService::initCache(tuner.enableInMemCache);

    app()
        .addListener("0.0.0.0", config.port)
        .setThreadNum(tuner.threadNum)
        .setIdleConnectionTimeout(10000)
        .setServerHeaderField("")
        .setLogLevel(trantor::Logger::kFatal);

    // Using the explicit 13-parameter signature for Drogon 1.9.12
    // This is the most stable way to ensure parameters are passed correctly to libpq
    app().createDbClient("postgresql", 
                         config.dbHost, 
                         (unsigned short)config.dbPort, 
                         config.dbName, 
                         config.dbUser, 
                         config.dbPassword, 
                         (size_t)tuner.dbConnections,
                         "",       // Connect string (empty when using params)
                         "default",// Client name
                         true,     // isFast
                         "",       // characterSet
                         0.0,      // timeout
                         false);   // autoBatch

    app().registerHandler("/health-check", [](HttpRequestPtr req) -> Task<HttpResponsePtr> {
        return PersonHandler::handleHealthCheck(req);
    });

    app().registerHandler("/pessoas", [](HttpRequestPtr req) -> Task<HttpResponsePtr> {
        return PersonHandler::handlePost(req);
    }, {Post});

    app().registerHandler("/pessoas/{id}", [](HttpRequestPtr req, std::string id) -> Task<HttpResponsePtr> {
        return PersonHandler::handleGetById(req, id);
    }, {Get});

    app().registerHandler("/pessoas?t={search}", [](HttpRequestPtr req) -> Task<HttpResponsePtr> {
        return PersonHandler::handleSearch(req);
    }, {Get});

    app().registerHandler("/contagem-pessoas", [](HttpRequestPtr req) -> Task<HttpResponsePtr> {
        return PersonHandler::handleCount(req);
    }, {Get});

    app().run();

    return 0;
}
