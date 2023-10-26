#include <drogon/drogon.h>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include "responses.cc"
#include "utils.cc"
#include "handlers/get-health-check.cc"
#include "handlers/post-pessoas.cc"
#include "handlers/get-pessoas-by-id.cc"
#include "handlers/get-pessoas-by-term.cc"
#include "handlers/get-contagem-pessoas.cc"

using namespace std;
using namespace drogon;

int main()
{
    const auto port(atoi(getenv("PORT")));
    const auto dbMaxConnections(atol(getenv("DB_MAX_CONNECTIONS")));
    const auto numThreads(atoi(getenv("NUM_THREADS")));

    const char *dbHost = getenv("DB_HOST");
    const auto dbPort(atoi(getenv("DB_PORT")));
    const char *dbName = getenv("DB_NAME");
    const char *dbUser = getenv("DB_USER");
    const char *dbPassword = getenv("DB_PASSWORD");

    logInfo("Starting backend-cockfighting-api server on http://localhost:" + to_string(port));

    app()
        .addListener("0.0.0.0", port)
        .setThreadNum(numThreads)
        .setIdleConnectionTimeout(10000);

    app().createDbClient("postgresql", dbHost, dbPort, dbName, dbUser, dbPassword, dbMaxConnections);

    app().registerHandler("/health-check", getHealthCheck())
        .registerHandler("/pessoas", postPessoas(), {Post})
        .registerHandler("/pessoas/{id}", getPessoasById(), {Get})
        .registerHandler("/pessoas?t={search}", getPessoasByTerm(), {Get})
        .registerHandler("/contagem-pessoas", getContagemPessoas(), {Get})
        .run();

    return 0;
}
