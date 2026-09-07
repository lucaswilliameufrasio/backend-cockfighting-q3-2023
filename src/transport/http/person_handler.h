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
            co_return makeResponse(drogon::k400BadRequest, "Empty");
        }

        domain::Person p;
        try {
            simdjson::padded_string padded_body(body);
            auto doc = json_parser.iterate(padded_body);
            p.nickname = std::string(doc["apelido"].get_string().value());
            p.name = std::string(doc["nome"].get_string().value());
            p.birth_date = std::string(doc["nascimento"].get_string().value());

            auto stack_field = doc["stack"];
            auto array_res = stack_field.get_array();
            if (!array_res.error()) {
                auto array = array_res.value();
                std::vector<std::string> stacks;
                for (auto elem : array) {
                    auto sv = elem.get_string();
                    if (sv.error()) {
                        co_return makeResponse(drogon::k400BadRequest, "Stack type");
                    }
                    stacks.emplace_back(std::string(sv.value()));
                }
                p.stack = std::move(stacks);
            }
        } catch (...) {
            co_return makeResponse(drogon::k400BadRequest, "Invalid JSON");
        }

        if (p.nickname.empty() || p.name.empty() || !config::isDateValid(p.birth_date)) {
            co_return makeResponse(drogon::k422UnprocessableEntity, "Invalid");
        }
        if (p.nickname.size() > 32 || p.name.size() > 100) {
            co_return makeResponse(drogon::k422UnprocessableEntity, "Too long");
        }
        if (p.stack.has_value()) {
            for (const auto &s : *p.stack) {
                if (s.size() > 32) {
                    co_return makeResponse(drogon::k422UnprocessableEntity, "Too long");
                }
            }
        }

        auto id = co_await application::PersonService::createPerson(p);
        if (!id) {
            co_return makeResponse(drogon::k422UnprocessableEntity, "Conflict");
        }

        std::string res_body = "{\"id\":";
        appendQuoted(res_body, *id);
        res_body += '}';
        auto resp = jsonBodyResponse(drogon::k201Created, std::move(res_body));
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
        co_return jsonBodyResponse(drogon::k200OK, personToJson(*person));
    }

    static drogon::Task<drogon::HttpResponsePtr> handleSearch(drogon::HttpRequestPtr req) {
        const auto &term = req->getParameter("t");
        if (term.empty()) {
            co_return makeResponse(drogon::k400BadRequest, "Missing term");
        }

        auto people = co_await application::PersonService::searchPeople(term);
        std::string body = "[";
        body.reserve(64 + people.size() * 192);
        for (size_t i = 0; i < people.size(); ++i) {
            if (i > 0) {
                body += ',';
            }
            body += personToJson(people[i]);
        }
        body += ']';
        co_return jsonBodyResponse(drogon::k200OK, std::move(body));
    }

    static drogon::Task<drogon::HttpResponsePtr> handleCount(drogon::HttpRequestPtr req) {
        size_t c = co_await application::PersonService::getPersonCount();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setBody(std::to_string(c));
        co_return resp;
    }

    static drogon::Task<drogon::HttpResponsePtr> handleHealthCheck(drogon::HttpRequestPtr req) {
        co_return drogon::HttpResponse::newHttpResponse();
    }
};

} // namespace transport::http
