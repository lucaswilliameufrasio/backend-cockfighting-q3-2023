#pragma once
#include <drogon/drogon.h>
#include <string>
#include <vector>

namespace transport::http {

inline drogon::HttpResponsePtr makeResponse(drogon::HttpStatusCode code, const std::string &msg) {
    Json::Value json;
    json["message"] = msg;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
    resp->setStatusCode(code);
    return resp;
}

} // namespace transport::http
