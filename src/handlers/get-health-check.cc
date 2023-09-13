#include <drogon/drogon.h>

using namespace std;
using namespace drogon;

auto getHealthCheck()
{
  return [](const HttpRequestPtr &req,
            function<void(const HttpResponsePtr &)> &&callback)
  {
    Json::Value json;
    json["message"] = "ok";
    auto resp = HttpResponse::newHttpJsonResponse(json);
    callback(resp);
  };
}
