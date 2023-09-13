#include <drogon/drogon.h>

using namespace std;
using namespace drogon;

HttpResponsePtr makeBadRequestResponse(string message)
{
  Json::Value json;
  json["message"] = message;
  auto resp = HttpResponse::newHttpJsonResponse(json);
  resp->setStatusCode(k400BadRequest);
  return resp;
}

HttpResponsePtr makeNotFoundResponseResponse(string message)
{
  Json::Value json;
  json["message"] = message;
  auto resp = HttpResponse::newHttpJsonResponse(json);
  resp->setStatusCode(k404NotFound);
  return resp;
}

HttpResponsePtr makeUnprocessableContentResponseResponse(string message)
{
  Json::Value json;
  json["message"] = message;
  auto resp = HttpResponse::newHttpJsonResponse(json);
  resp->setStatusCode(k422UnprocessableEntity);
  return resp;
}

HttpResponsePtr makeFailedResponse()
{
  Json::Value json;
  json["message"] = "Manda o negócio direito porque deu bom não";
  auto resp = HttpResponse::newHttpJsonResponse(json);
  resp->setStatusCode(k500InternalServerError);
  return resp;
}
