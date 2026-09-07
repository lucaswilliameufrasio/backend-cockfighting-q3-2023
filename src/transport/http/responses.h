#pragma once
#include <drogon/drogon.h>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace transport::http {

// Montagem manual de JSON (sem Json::Value) para reduzir o CPU por request:
// sob quota de 0.25 CPU, alocações do jsoncpp causavam throttling de CFS
// e picos bimodais de ~50ms.

inline void jsonEscapeAppend(std::string &out, const std::string &s) {
    for (const unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
}

inline void appendQuoted(std::string &out, const std::string &s) {
    out += '"';
    jsonEscapeAppend(out, s);
    out += '"';
}

inline std::string personToJson(const domain::Person &p) {
    std::string out;
    out.reserve(192);
    out += "{\"id\":";
    appendQuoted(out, p.id);
    out += ",\"apelido\":";
    appendQuoted(out, p.nickname);
    out += ",\"nome\":";
    appendQuoted(out, p.name);
    out += ",\"nascimento\":";
    appendQuoted(out, p.birth_date);
    out += ",\"stack\":";
    if (p.stack && !p.stack->empty()) {
        out += '[';
        for (size_t i = 0; i < p.stack->size(); ++i) {
            if (i > 0) {
                out += ',';
            }
            appendQuoted(out, (*p.stack)[i]);
        }
        out += ']';
    } else {
        out += "null";
    }
    out += '}';
    return out;
}

inline drogon::HttpResponsePtr makeResponse(drogon::HttpStatusCode code, const std::string &msg) {
    std::string body = "{\"message\":";
    appendQuoted(body, msg);
    body += '}';
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(code);
    resp->setBody(std::move(body));
    resp->addHeader("Content-Type", "application/json");
    return resp;
}

inline drogon::HttpResponsePtr jsonBodyResponse(drogon::HttpStatusCode code, std::string body) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(code);
    resp->setBody(std::move(body));
    resp->addHeader("Content-Type", "application/json");
    return resp;
}

} // namespace transport::http
