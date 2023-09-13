#include <drogon/drogon.h>

using namespace std;
using namespace drogon;

auto getPessoasById()
{
  return [](const HttpRequestPtr &req,
            function<void(const HttpResponsePtr &)> &&callback,
            const string &id)
  {
    if (id.size() != 36)
    {
      callback(makeNotFoundResponseResponse("This person do not exist."));
      return;
    }

    auto clientPtr = drogon::app().getDbClient();

    auto f = clientPtr->execSqlAsyncFuture("SELECT people.id, people.nickname, people.name, people.birth_date, people.stack FROM people WHERE people.id = $1;", id);

    try
    {
      auto result = f.get();

      if (result.size() == 0)
      {
        callback(makeNotFoundResponseResponse("This person do not exist."));
        return;
      }

      Json::Value json;

      stringstream stackStream(result[0]["stack"].as<string>());

      string split;
      int position = 0;
      while (getline(stackStream, split, ','))
      {
        json["stack"][position] = split;
        position++;
      }

      json["id"] = result[0]["id"].as<string>();
      json["apelido"] = result[0]["nickname"].as<string>();
      json["nome"] = result[0]["name"].as<string>();
      json["nascimento"] = result[0]["birth_date"].as<string>();

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
