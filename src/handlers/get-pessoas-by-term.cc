#include <drogon/drogon.h>

using namespace std;
using namespace drogon;

auto getPessoasByTerm()
{
  return [](const HttpRequestPtr &req,
            function<void(const HttpResponsePtr &)> &&callback,
            const string &search)
  {
    if (search.empty())
    {
      callback(makeBadRequestResponse("The query parameter 't' is required"));
      return;
    }

    auto clientPtr = drogon::app().getDbClient();

    auto f = clientPtr->execSqlAsyncFuture("SELECT people.id, people.nickname, people.name, people.birth_date, people.stack FROM people WHERE LOWER(people.name || ' ' || people.nickname || ' ' || people.stack) LIKE LOWER($1) LIMIT 50;", "%" + search + "%");

    try
    {
      auto result = f.get();

      Json::Value json = Json::arrayValue;

      if (result.size() == 0)
      {
        auto resp = HttpResponse::newHttpJsonResponse(json);
        resp->setStatusCode(drogon::HttpStatusCode::k200OK);
        callback(resp);
        return;
      }

      int i = 0;
      for (auto row : result)
      {
        stringstream stackStream(result[i]["stack"].as<string>());

        string split;
        int position = 0;
        while (getline(stackStream, split, ','))
        {
          json[i]["stack"][position] = split;
          position++;
        }

        json[i]["id"] = result[i]["id"].as<string>();
        json[i]["apelido"] = result[i]["nickname"].as<string>();
        json[i]["nome"] = result[i]["name"].as<string>();
        json[i]["nascimento"] = result[i]["birth_date"].as<string>();
        i++;
      }

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
