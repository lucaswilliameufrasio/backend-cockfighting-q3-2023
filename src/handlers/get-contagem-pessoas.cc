#include <drogon/drogon.h>

using namespace std;
using namespace drogon;

auto getContagemPessoas()
{
  return [](const HttpRequestPtr &req,
            function<void(const HttpResponsePtr &)> &&callback)
  {
    auto clientPtr = drogon::app().getDbClient();

    auto f = clientPtr->execSqlAsyncFuture("SELECT COUNT(*) AS count FROM people;");

    try
    {
      auto result = f.get();

      Json::Value json = Json::objectValue;

      if (result.size() == 0)
      {
        json["count"] = 0;
        auto resp = HttpResponse::newHttpJsonResponse(json);
        resp->setStatusCode(drogon::HttpStatusCode::k200OK);
        callback(resp);
        return;
      }

      json["count"] = result[0]["count"].as<int>();

      auto resp = HttpResponse::newHttpJsonResponse(json);
      resp->setStatusCode(drogon::HttpStatusCode::k200OK);
      callback(resp);

      return;
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
      logException(e.base().what());

      callback(makeFailedResponse());
      return;
    }
  };
}
