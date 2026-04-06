#include "presentation/http/auth/AuthController.hpp"
#include "presentation/http/auth/AuthSchemas.hpp"
#include "presentation/http/ResponseHelper.hpp"
#include "core/validation/RequestValidator.hpp"
#include "infrastructure/auth/GoogleAuthClient.hpp"
#include "infrastructure/config/EnvironmentConfig.hpp"
#include "infrastructure/email/ConsoleEmailSender.hpp"
#include "infrastructure/email/SmtpEmailSender.hpp"
#include "infrastructure/persistence/PostgresRefreshTokenRepository.hpp"
#include "infrastructure/persistence/PostgresUserRepository.hpp"
#include "infrastructure/persistence/PostgresVerificationTokenRepository.hpp"
#include "infrastructure/security/ArgonPasswordHasher.hpp"
#include "infrastructure/security/JwtTokenService.hpp"
#include "infrastructure/util/DrogonUuidGenerator.hpp"

#include <nlohmann/json.hpp>
#include <sodium.h>
#include <spdlog/spdlog.h>

namespace Nyx::Presentation::Http::Auth {
  AuthController::AuthController() {
    auto config = std::make_shared<
      Nyx::Infrastructure::Config::EnvironmentConfig
    >();

    auto user_repository = std::make_shared<
      Nyx::Infrastructure::Persistence::PostgresUserRepository
    >();

    auto refresh_token_repository = std::make_shared<
      Nyx::Infrastructure::Persistence::PostgresRefreshTokenRepository
    >();

    auto password_hasher = std::make_shared<
      Nyx::Infrastructure::Security::ArgonPasswordHasher
    >();

    auto token_service = std::make_shared<
      Nyx::Infrastructure::Security::JwtTokenService
    >(config);

    auto uuid_generator = std::make_shared<
      Nyx::Infrastructure::Util::DrogonUuidGenerator
    >();

    auto verification_token_repository =
      std::make_shared<
        Nyx::Infrastructure::Persistence::PostgresVerificationTokenRepository
      >();

    auto email_sender = [&config]()
      -> std::shared_ptr<
        Nyx::Application::Auth::IEmailSender
      > {
      if (!config->smtp_host().empty()) {
        return std::make_shared<
          Nyx::Infrastructure::Email::SmtpEmailSender
        >(config);
      }
      return std::make_shared<
        Nyx::Infrastructure::Email::ConsoleEmailSender
      >(config);
    }();

    auto google_auth_client = std::make_shared<
      Nyx::Infrastructure::Auth::GoogleAuthClient
    >(config);

    this->auth_service = std::make_shared<
      Nyx::Application::Auth::AuthService
    >(
      user_repository,
      refresh_token_repository,
      password_hasher,
      token_service,
      uuid_generator,
      verification_token_repository,
      email_sender,
      google_auth_client
    );

    this->cookie_secure = config->cookie_secure();
    this->refresh_token_max_age_seconds =
      config->jwt_refresh_token_expiry_seconds();

    spdlog::debug("AuthController initialized");
  }

  auto AuthController::generate_csrf_token()
    -> std::string {
    unsigned char buffer[32];
    randombytes_buf(buffer, sizeof(buffer));

    char hex[sizeof(buffer) * 2 + 1];
    sodium_bin2hex(
      hex, sizeof(hex), buffer, sizeof(buffer)
    );

    return std::string(hex);
  }

  auto AuthController::register_user(
    const drogon::HttpRequestPtr& request,
    std::function<
      void(const drogon::HttpResponsePtr&)
    >&& callback
  ) -> void {
    auto correlation_id =
      request->getAttributes()
        ->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()
        ->get<std::shared_ptr<spdlog::logger>>(
          "logger"
        );

    if (!logger) {
      logger = spdlog::default_logger();
    }

    nlohmann::json request_body;
    try {
      request_body = nlohmann::json::parse(request->body());
    } catch (
      const nlohmann::json::parse_error& exception
    ) {
      logger->warn(
        "Invalid JSON in register request: {}",
        exception.what()
      );

      auto error = Nyx::Core::AppError{
        Nyx::Core::ErrorCode::InvalidJson,
        "Request body must be valid JSON",
        {}
      };

      callback(ResponseHelper::error(
        error, correlation_id
      ));
      return;
    }

    auto validated = Nyx::Core::RequestValidator::validate(
			request_body, register_request_schema
		);

    if (!validated.has_value()) {
      logger->warn(
        "Validation failed for register request"
      );
      callback(ResponseHelper::error(
        validated.error(), correlation_id
      ));
      return;
    }

    auto register_request = Nyx::Application::Auth::RegisterRequest{
			.email = request_body["email"].get<std::string>(),
			.password = request_body["password"].get<std::string>(),
		};

    auto result = this->auth_service->register_user(
			register_request, logger
		);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"id", result->id},
      {"email", result->email},
      {"email_verified", result->email_verified},
    };

    callback(ResponseHelper::created(
      response_data, correlation_id
    ));
  }

  auto AuthController::login(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
  ) -> void {
    auto correlation_id = request->getAttributes()->get<std::string>("correlation_id");
    auto logger = request->getAttributes()->get<std::shared_ptr<spdlog::logger>>(
			"logger"
		);

    if (!logger) {
      logger = spdlog::default_logger();
    }

    nlohmann::json request_body;
    try {
      request_body = nlohmann::json::parse(request->body());
    } catch (
      const nlohmann::json::parse_error& exception
    ) {
      logger->warn("Invalid JSON in login request: {}", exception.what());

      auto error = Nyx::Core::AppError{
        Nyx::Core::ErrorCode::InvalidJson,
        "Request body must be valid JSON",
        {}
      };

      callback(ResponseHelper::error(error, correlation_id));
      return;
    }

    auto validated = Nyx::Core::RequestValidator::validate(
			request_body, login_request_schema
		);

    if (!validated.has_value()) {
      logger->warn("Validation failed for login request");
      callback(ResponseHelper::error(validated.error(), correlation_id));
      return;
    }

    auto login_request =Nyx::Application::Auth::LoginRequest{
			.email = request_body["email"].get<std::string>(),
			.password = request_body["password"].get<std::string>(),
    };

		auto result = this->auth_service->login(
			login_request, logger
		);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto csrf_token = this->generate_csrf_token();

    auto response_data = nlohmann::json{
      {"access_token", result->access_token},
      {"csrf_token", csrf_token},
    };

    auto response = ResponseHelper::success(
      response_data, correlation_id
    );
    ResponseHelper::set_refresh_token_cookie(
      response, result->refresh_token,
      static_cast<int>(this->refresh_token_max_age_seconds),
      this->cookie_secure
    );
    ResponseHelper::set_csrf_cookie(
      response, csrf_token, this->cookie_secure
    );
    callback(response);
  }

  auto AuthController::refresh(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
  ) -> void {
    auto correlation_id =request->getAttributes()->get<std::string>("correlation_id");
    auto logger = request->getAttributes()->get<std::shared_ptr<spdlog::logger>>("logger");

    if (!logger) {
      logger = spdlog::default_logger();
    }

    auto refresh_token = request->getCookie("refresh_token");

    if (refresh_token.empty()) {
      logger->warn("No refresh token cookie present");

      auto error = Nyx::Core::AppError::unauthorized(
        "Refresh token is required"
      );
      callback(ResponseHelper::error(
        error, correlation_id
      ));
      return;
    }

    auto result = this->auth_service->refresh_access_token(
      refresh_token, logger
    );

    if (!result.has_value()) {
      auto response = ResponseHelper::error(
        result.error(), correlation_id
      );
      ResponseHelper::clear_refresh_token_cookie(
        response, this->cookie_secure
      );
      ResponseHelper::clear_csrf_cookie(
        response, this->cookie_secure
      );
      callback(response);
      return;
    }

    auto csrf_token = this->generate_csrf_token();

    auto response_data = nlohmann::json{
      {"access_token", result->access_token},
      {"csrf_token", csrf_token},
    };

    auto response = ResponseHelper::success(
      response_data, correlation_id
    );
    ResponseHelper::set_refresh_token_cookie(
      response, result->refresh_token,
      static_cast<int>(this->refresh_token_max_age_seconds),
      this->cookie_secure
    );
    ResponseHelper::set_csrf_cookie(
      response, csrf_token, this->cookie_secure
    );
    callback(response);
  }

  auto AuthController::logout(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
  ) -> void {
    auto correlation_id = request->getAttributes()->get<std::string>("correlation_id");
    auto logger = request->getAttributes()->get<std::shared_ptr<spdlog::logger>>("logger");

    if (!logger) {
      logger = spdlog::default_logger();
    }

    auto refresh_token = request->getCookie("refresh_token");

    if (refresh_token.empty()) {
      logger->warn("No refresh token cookie present for logout");

      auto error = Nyx::Core::AppError::unauthorized(
        "Refresh token is required"
      );
      callback(ResponseHelper::error(
        error, correlation_id
      ));
      return;
    }

    auto result = this->auth_service->logout(
      refresh_token, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    auto response = ResponseHelper::no_content();
    ResponseHelper::clear_refresh_token_cookie(
      response, this->cookie_secure
    );
    ResponseHelper::clear_csrf_cookie(
      response, this->cookie_secure
    );
    callback(response);
  }
  auto AuthController::verify_email(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
  ) -> void {
    auto correlation_id =
      request->getAttributes()
        ->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()
        ->get<std::shared_ptr<spdlog::logger>>(
          "logger"
        );

    if (!logger) {
      logger = spdlog::default_logger();
    }

    nlohmann::json request_body;
    try {
      request_body =
        nlohmann::json::parse(request->body());
    } catch (
      const nlohmann::json::parse_error& exception
    ) {
      logger->warn(
        "Invalid JSON in verify-email request: {}",
        exception.what()
      );
      callback(ResponseHelper::error(
        Nyx::Core::AppError{
          Nyx::Core::ErrorCode::InvalidJson,
          "Request body must be valid JSON",
          {}
        },
        correlation_id
      ));
      return;
    }

    auto validated =
      Nyx::Core::RequestValidator::validate(
        request_body, verify_email_schema
      );

    if (!validated.has_value()) {
      callback(ResponseHelper::error(
        validated.error(), correlation_id
      ));
      return;
    }

    auto token =
      request_body["token"].get<std::string>();

    auto result = this->auth_service->verify_email(
      token, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    callback(ResponseHelper::success(
      nlohmann::json{
        {"message", "Email verified successfully"}
      },
      correlation_id
    ));
  }

  auto AuthController::resend_verification(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
  ) -> void {
    auto correlation_id =
      request->getAttributes()
        ->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()
        ->get<std::shared_ptr<spdlog::logger>>(
          "logger"
        );

    if (!logger) {
      logger = spdlog::default_logger();
    }

    nlohmann::json request_body;
    try {
      request_body =
        nlohmann::json::parse(request->body());
    } catch (
      const nlohmann::json::parse_error& exception
    ) {
      logger->warn(
        "Invalid JSON in resend-verification "
        "request: {}",
        exception.what()
      );
      callback(ResponseHelper::error(
        Nyx::Core::AppError{
          Nyx::Core::ErrorCode::InvalidJson,
          "Request body must be valid JSON",
          {}
        },
        correlation_id
      ));
      return;
    }

    auto validated =
      Nyx::Core::RequestValidator::validate(
        request_body, resend_verification_schema
      );

    if (!validated.has_value()) {
      callback(ResponseHelper::error(
        validated.error(), correlation_id
      ));
      return;
    }

    auto email =
      request_body["email"].get<std::string>();

    auto result =
      this->auth_service->resend_verification(
        email, logger
      );

    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    callback(ResponseHelper::success(
      nlohmann::json{
        {"message",
          "If an account exists with this email, "
          "a verification link has been sent"}
      },
      correlation_id
    ));
  }

  auto AuthController::google_login(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
  ) -> void {
    auto correlation_id =
      request->getAttributes()
        ->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()
        ->get<std::shared_ptr<spdlog::logger>>(
          "logger"
        );

    if (!logger) {
      logger = spdlog::default_logger();
    }

    nlohmann::json request_body;
    try {
      request_body =
        nlohmann::json::parse(request->body());
    } catch (
      const nlohmann::json::parse_error& exception
    ) {
      logger->warn(
        "Invalid JSON in google login request: {}",
        exception.what()
      );
      callback(ResponseHelper::error(
        Nyx::Core::AppError{
          Nyx::Core::ErrorCode::InvalidJson,
          "Request body must be valid JSON",
          {}
        },
        correlation_id
      ));
      return;
    }

    auto validated =
      Nyx::Core::RequestValidator::validate(
        request_body, google_login_schema
      );

    if (!validated.has_value()) {
      callback(ResponseHelper::error(
        validated.error(), correlation_id
      ));
      return;
    }

    auto google_request =
      Nyx::Application::Auth::GoogleLoginRequest{
        .code =
          request_body["code"].get<std::string>(),
        .redirect_uri =
          request_body["redirect_uri"]
            .get<std::string>(),
      };

    auto result =
      this->auth_service->google_login(
        google_request, logger
      );

    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    auto csrf_token = this->generate_csrf_token();

    auto response_data = nlohmann::json{
      {"access_token", result->access_token},
      {"csrf_token", csrf_token},
    };

    auto response = ResponseHelper::success(
      response_data, correlation_id
    );
    ResponseHelper::set_refresh_token_cookie(
      response, result->refresh_token,
      static_cast<int>(
        this->refresh_token_max_age_seconds
      ),
      this->cookie_secure
    );
    ResponseHelper::set_csrf_cookie(
      response, csrf_token, this->cookie_secure
    );
    callback(response);
  }
} // namespace Nyx::Presentation::Http::Auth
