#include "infrastructure/auth/GoogleAuthClient.hpp"

#include <drogon/HttpClient.h>
#include <jwt-cpp/traits/nlohmann-json/defaults.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Auth {
  GoogleAuthClient::GoogleAuthClient(
    std::shared_ptr<
      Nyx::Infrastructure::Config::EnvironmentConfig
    > config
  ) : config(std::move(config)) {}

  auto GoogleAuthClient::exchange_code(
    const std::string& code,
    const std::string& redirect_uri
  ) -> Nyx::Core::Result<
    Nyx::Application::Auth::GoogleUserInfo
  > {
    auto client_id = this->config->google_client_id();
    auto client_secret =
      this->config->google_client_secret();

    if (client_id.empty()
      || client_secret.empty()) {
      spdlog::error(
        "Google OAuth not configured: missing "
        "GOOGLE_CLIENT_ID or GOOGLE_CLIENT_SECRET"
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Google authentication is not configured"
        )
      );
    }

    try {
      auto http_client =
        drogon::HttpClient::newHttpClient(
          "https://oauth2.googleapis.com"
        );
      http_client->setSockOptCallback(
        [](int) {}
      );

      auto request = drogon::HttpRequest::newHttpRequest();
      request->setMethod(drogon::Post);
      request->setPath("/token");
      request->setContentTypeCode(
        drogon::CT_APPLICATION_X_FORM
      );
      request->setBody(
        "code=" + code
        + "&client_id=" + client_id
        + "&client_secret=" + client_secret
        + "&redirect_uri=" + redirect_uri
        + "&grant_type=authorization_code"
      );

      auto [result, response] =
        http_client->sendRequest(request, 10.0);

      if (result != drogon::ReqResult::Ok
        || response == nullptr) {
        spdlog::error(
          "Google token exchange HTTP request failed"
        );
        return std::unexpected(
          Nyx::Core::AppError{
            Nyx::Core::ErrorCode::ExternalServiceError,
            "Failed to exchange Google "
            "authorization code",
            {}
          }
        );
      }

      if (response->statusCode()
        != drogon::k200OK) {
        spdlog::error(
          "Google token exchange returned status={}, "
          "body={}",
          static_cast<int>(response->statusCode()),
          std::string(response->body())
        );
        return std::unexpected(
          Nyx::Core::AppError{
            Nyx::Core::ErrorCode::ExternalServiceError,
            "Google authentication failed",
            {}
          }
        );
      }

      auto response_body = nlohmann::json::parse(
        response->body()
      );

      if (!response_body.contains("id_token")) {
        spdlog::error(
          "Google token response missing id_token"
        );
        return std::unexpected(
          Nyx::Core::AppError{
            Nyx::Core::ErrorCode::ExternalServiceError,
            "Invalid response from Google",
            {}
          }
        );
      }

      auto id_token_str =
        response_body["id_token"].get<std::string>();
      auto decoded_token = jwt::decode(id_token_str);

      auto google_id =
        decoded_token.get_payload_claim("sub")
          .as_string();
      auto email =
        decoded_token.get_payload_claim("email")
          .as_string();
      auto email_verified_claim =
        decoded_token.get_payload_claim(
          "email_verified"
        ).to_json();
      auto email_verified =
        email_verified_claim.get<bool>();

      auto name = std::optional<std::string>{};
      if (decoded_token.has_payload_claim("name")) {
        name = decoded_token.get_payload_claim("name")
          .as_string();
      }

      return Nyx::Application::Auth::GoogleUserInfo{
        .google_id = std::move(google_id),
        .email = std::move(email),
        .email_verified = email_verified,
        .name = std::move(name),
      };
    } catch (const std::exception& exception) {
      spdlog::error(
        "Google OAuth error: {}", exception.what()
      );
      return std::unexpected(
        Nyx::Core::AppError{
          Nyx::Core::ErrorCode::ExternalServiceError,
          "Google authentication failed",
          {}
        }
      );
    }
  }
} // namespace Nyx::Infrastructure::Auth
