#pragma once

#include "core/error/AppError.hpp"

#include <drogon/HttpResponse.h>
#include <nlohmann/json.hpp>
#include <trantor/utils/Date.h>

namespace Nyx::Presentation::Http {
  class ResponseHelper {
    public:
      static auto success(
        const nlohmann::json& data,
        const std::string& correlation_id,
        drogon::HttpStatusCode status_code =
          drogon::k200OK
      ) -> drogon::HttpResponsePtr {
        auto meta = nlohmann::json{
          {"request_id", correlation_id},
          {"timestamp",
            trantor::Date::now()
              .toFormattedString(false)}
        };

        auto response_body = nlohmann::json{
          {"data", data},
          {"meta", meta}
        };

        return make_json_response(
          response_body, status_code
        );
      }

      static auto created(
        const nlohmann::json& data,
        const std::string& correlation_id
      ) -> drogon::HttpResponsePtr {
        return success(
          data, correlation_id, drogon::k201Created
        );
      }

      static auto no_content()
        -> drogon::HttpResponsePtr {
        auto response =
          drogon::HttpResponse::newHttpResponse();
        response->setStatusCode(
          drogon::k204NoContent
        );
        return response;
      }

      static auto error(
        const Nyx::Core::AppError& application_error,
        const std::string& correlation_id
      ) -> drogon::HttpResponsePtr {
        auto error_body = nlohmann::json{
          {"code",
            application_error.error_code_string()},
          {"message", application_error.message}
        };

        if (!application_error.details.empty()) {
          auto details_array =
            nlohmann::json::array();
          for (const auto& detail :
               application_error.details) {
            details_array.push_back(nlohmann::json{
              {"field", detail.field},
              {"message", detail.message}
            });
          }
          error_body["details"] = details_array;
        }

        auto meta = nlohmann::json{
          {"request_id", correlation_id},
          {"timestamp",
            trantor::Date::now()
              .toFormattedString(false)}
        };

        auto response_body = nlohmann::json{
          {"error", error_body},
          {"meta", meta}
        };

        return make_json_response(
          response_body,
          static_cast<drogon::HttpStatusCode>(
            application_error.http_status_code()
          )
        );
      }

      static auto set_refresh_token_cookie(
        drogon::HttpResponsePtr& response,
        const std::string& refresh_token,
        int max_age_seconds,
        bool secure
      ) -> void {
        auto cookie = drogon::Cookie("refresh_token", refresh_token);
        cookie.setHttpOnly(true);
        cookie.setSecure(secure);
        cookie.setSameSite(drogon::Cookie::SameSite::kStrict);
        cookie.setPath("/api/v1/auth");
        cookie.setMaxAge(max_age_seconds);
        response->addCookie(cookie);
      }

      static auto clear_refresh_token_cookie(
        drogon::HttpResponsePtr& response,
        bool secure
      ) -> void {
        auto cookie = drogon::Cookie("refresh_token", "");
        cookie.setHttpOnly(true);
        cookie.setSecure(secure);
        cookie.setSameSite(drogon::Cookie::SameSite::kStrict);
        cookie.setPath("/api/v1/auth");
        cookie.setMaxAge(0);
        response->addCookie(cookie);
      }

      static auto set_csrf_cookie(
        drogon::HttpResponsePtr& response,
        const std::string& csrf_token,
        bool secure
      ) -> void {
        auto cookie = drogon::Cookie("csrf_token", csrf_token);
        cookie.setHttpOnly(false);
        cookie.setSecure(secure);
        cookie.setSameSite(drogon::Cookie::SameSite::kStrict);
        cookie.setPath("/");
        cookie.setMaxAge(604800);
        response->addCookie(cookie);
      }

      static auto clear_csrf_cookie(
        drogon::HttpResponsePtr& response,
        bool secure
      ) -> void {
        auto cookie = drogon::Cookie("csrf_token", "");
        cookie.setHttpOnly(false);
        cookie.setSecure(secure);
        cookie.setSameSite(drogon::Cookie::SameSite::kStrict);
        cookie.setPath("/");
        cookie.setMaxAge(0);
        response->addCookie(cookie);
      }

    private:
      static auto make_json_response(
        const nlohmann::json& body,
        drogon::HttpStatusCode status_code
      ) -> drogon::HttpResponsePtr {
        auto response =
          drogon::HttpResponse::newHttpResponse();
        response->setStatusCode(status_code);
        response->setContentTypeCode(
          drogon::CT_APPLICATION_JSON
        );
        response->setBody(body.dump());
        return response;
      }
  };
} // namespace Nyx::Presentation::Http
