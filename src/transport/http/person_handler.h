#pragma once
#include <drogon/drogon.h>
#include <simdjson.h>
#include "../../application/person_service.h"
#include "../../config/app_config.h"
#include "../../config/resource_tuner.h"
#include "responses.h"

namespace transport::http {

thread_local simdjson::ondemand::parser json_parser;
static config::TuningProfile current_tuner;

struct PersonHandler {
    static void setTuner(const config::TuningProfile &t) { current_tuner = t; }

    static drogon::Task<drogon::HttpResponsePtr> handlePost(drogon::HttpRequestPtr req) {
        auto body = req->getBody();
        if (body.empty()) {
            co_return makeResponse(drogon::k500InternalServerError, "Error");
        }

        domain::Person p;
        try {
            simdjson::padded_string padded_body(body);
            auto doc = json_parser.iterate(padded_body);
            p.nickname = std::string(doc["apelido"].get_string().value());
            p.name = std::string(doc["nome"].get_string().value());
            p.birth_date = std::string(doc["nascimento"].get_string().value());

            if (p.nickname.empty() || p.name.empty() || !config::isDateValid(p.birth_date)) {
                co_return makeResponse(drogon::k422UnprocessableEntity, "Invalid");
            }
            if (p.nickname.size() > 32 || p.name.size() > 100) {
                co_return makeResponse(drogon::k422UnprocessableEntity, "Too long");
            }

            auto stack_field = doc["stack"];
            if (stack_field.type() == simdjson::ondemand::json_type::array) {
                std::vector<std::string> stacks;
                for (auto s : stack_field.get_array()) {
                    std::string_view sv = s.get_string().value();
                    if (sv.size() > 32) {
                        co_return makeResponse(drogon::k422UnprocessableEntity, "Too long");
                    }
                    stacks.emplace_back(sv);
                }
                p.stack = std::move(stacks);
            }
        } catch (...) {
            co_return makeResponse(drogon::k422UnprocessableEntity, "JSON Error");
        }

        auto id = co_await application::PersonService::createPerson(p);
        if (!id) {
            co_return makeResponse(drogon::k422UnprocessableEntity, "Conflict");
        }

        Json::Value res;
        res["id"] = *id;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
        resp->setStatusCode(drogon::k201Created);
        resp->addHeader("Location", "/pessoas/" + *id);
        co_return resp;
    }

    static drogon::Task<drogon::HttpResponsePtr> handleGetById(drogon::HttpRequestPtr req, std::string id) {
        if (id.size() != 36) {
            co_return makeResponse(drogon::k404NotFound, "Not found");
        }
        auto person = co_await application::PersonService::getPersonById(id, current_tuner.enableInMemCache);
        if (!person) {
            co_return makeResponse(drogon::k404NotFound, "Not found");
        }

        Json::Value res;
        res["id"] = person->id;
        res["apelido"] = person->nickname;
        res["nome"] = person->name;
        res["nascimento"] = person->birth_date;
        if (person->stack) {
            res["stack"] = Json::arrayValue;
            for (const auto &s : *person->stack) {
                res["stack"].append(s);
            }
        } else {
            res["stack"] = Json::nullValue;
        }
        co_return drogon::HttpResponse::newHttpJsonResponse(res);
    }

    static drogon::Task<drogon::HttpResponsePtr> handleSearch(drogon::HttpRequestPtr req) {
        const auto &term = req->getParameter("t");
        if (term.empty()) {
            co_return makeResponse(drogon::k400BadRequest, "Missing term");
        }

        auto people = co_await application::PersonService::searchPeople(term);
        Json::Value res = Json::arrayValue;
        for (const auto &p : people) {
            Json::Value pj;
            pj["id"] = p.id;
            pj["apelido"] = p.nickname;
            pj["nome"] = p.name;
            pj["nascimento"] = p.birth_date;
            if (p.stack) {
                pj["stack"] = Json::arrayValue;
                for (const auto &s : *p.stack) {
                    pj["stack"].append(s);
                }
            } else {
                pj["stack"] = Json::nullValue;
            }
            res.append(pj);
        }
        co_return drogon::HttpResponse::newHttpJsonResponse(res);
    }

    static drogon::Task<drogon::HttpResponsePtr> handleCount(drogon::HttpRequestPtr req) {
        size_t c = co_await application::PersonService::getPersonCount();
        Json::Value res;
        res["count"] = (unsigned int)c;
        co_return drogon::HttpResponse::newHttpJsonResponse(res);
    }

    static drogon::Task<drogon::HttpResponsePtr> handleHealthCheck(drogon::HttpRequestPtr req) {
        co_return drogon::HttpResponse::newHttpResponse();
    }
};

} // namespace transport::http
