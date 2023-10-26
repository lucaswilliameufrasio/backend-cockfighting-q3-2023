#include <drogon/drogon.h>

using namespace std;
using namespace drogon;

auto postPessoas()
{
  return [](const HttpRequestPtr &req,
            function<void(const HttpResponsePtr &)> &&callback)
  {
    auto jsonPtr = req->jsonObject();
    if (jsonPtr == nullptr)
    {
      callback(makeFailedResponse());
      return;
    }

    auto nickname = jsonPtr->get("apelido", "").as<string>();
    auto name = jsonPtr->get("nome", "").as<string>();
    auto birthDate = jsonPtr->get("nascimento", "").as<string>();
    auto stacksJson = jsonPtr->get("stack", "");

    if (nickname.size() == 0)
    {
      callback(makeUnprocessableContentResponseResponse("The 'apelido' parameter should not be empty"));
      return;
    }

    if (name.size() == 0)
    {
      callback(makeUnprocessableContentResponseResponse("The 'nome' parameter should not be empty"));
      return;
    }

    if (birthDate.size() == 0)
    {
      callback(makeUnprocessableContentResponseResponse("The 'nascimento' parameter should not be empty"));
      return;
    }

    if (stacksJson.size() == 0)
    {
      callback(makeUnprocessableContentResponseResponse("The 'stack' parameter should not be empty"));
      return;
    }

    if (nickname.size() > 32)
    {
      callback(makeUnprocessableContentResponseResponse("The 'apelido' parameter length should be less than 32 characters"));
      return;
    }

    if (name.size() > 100)
    {
      callback(makeUnprocessableContentResponseResponse("The 'apelido' parameter length should be less than 32 characters"));
      return;
    }

    if (birthDate.size() == 0 || !isDateValid(birthDate))
    {
      callback(makeUnprocessableContentResponseResponse("The 'nascimento' parameter is not a valid date of format 'YYYY-MM-DD'"));
      return;
    }

    string stacks;
    int invalidStack = 0;

    try
    {
      stacks.reserve(stacksJson.size());

      for (int i = 0; i < int(stacksJson.size()); ++i)
      {
        if (stacksJson[i].isNull() || stacksJson[i].isString() == 0)
        {
          invalidStack = 1;
          break;
        }

        string stack = stacksJson[i].asString();

        if (stack.size() > 32)
        {
          invalidStack = 2;
          break;
        }

        if (i > 0)
        {
          stacks = stacks + ",";
        }

        stacks = stacks + stack;
      }
    }
    catch (const exception &e)
    {
      logException(e.what());

      callback(makeFailedResponse());
      return;
    }

    if (invalidStack == 2)
    {
      callback(makeUnprocessableContentResponseResponse("The 'stack' parameter should have items with 32 characters or less"));
      return;
    }

    if (invalidStack == 1)
    {
      callback(makeUnprocessableContentResponseResponse("The 'stack' parameter should have only strings"));
      return;
    }

    auto clientPtr = drogon::app().getDbClient();

    auto personFound = clientPtr->execSqlAsyncFuture("SELECT id FROM people WHERE people.nickname = $1;", nickname);

    Json::Value json;

    try
    {
      auto result = personFound.get();

      if (result.size() > 0)
      {
        callback(makeUnprocessableContentResponseResponse("This person already exists."));
        return;
      }
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
      logException(e.base().what());

      callback(makeFailedResponse());
      return;
    }

    const std::string uuid = boost::uuids::to_string(boost::uuids::random_generator()());

    auto f = clientPtr->execSqlAsyncFuture("\
      INSERT INTO people (id, nickname, name, birth_date, stack) \
      VALUES ($1, $2, $3, $4, $5) \
      ON CONFLICT DO NOTHING \
      RETURNING id; \
      ",
      uuid, nickname, name, birthDate, stacks);

    string id = "";

    try
    {
      auto result = f.get();

      if (result.size() == 0)
      {
        callback(makeFailedResponse());
        return;
      }

      id = result[0]["id"].as<string>();
      json["id"] = id;
    }
    catch (const drogon::orm::DrogonDbException &e)
    {
      logException(e.base().what());

      callback(makeFailedResponse());
      return;
    }

    string location = "/pessoas/" + id;

    auto resp = HttpResponse::newHttpJsonResponse(json);
    resp->addHeader("Location", location);
    resp->setStatusCode(drogon::HttpStatusCode::k201Created);
    callback(resp);
  };
}
