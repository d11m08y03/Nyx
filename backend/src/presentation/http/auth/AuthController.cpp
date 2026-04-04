#include "presentation/http/auth/AuthController.hpp"
#include "presentation/http/auth/AuthSchemas.hpp"
#include "presentation/http/ResponseHelper.hpp"
#include "core/validation/RequestValidator.hpp"
#include "infrastructure/config/EnvironmentConfig.hpp"
#include "infrastructure/persistence/PostgresRefreshTokenRepository.hpp"
#include "infrastructure/persistence/PostgresUserRepository.hpp"
#include "infrastructure/security/ArgonPasswordHasher.hpp"
#include "infrastructure/security/JwtTokenService.hpp"
#include "infrastructure/util/DrogonUuidGenerator.hpp"

#include <nlohmann/json.hpp>
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

    this->auth_service = std::make_shared<
      Nyx::Application::Auth::AuthService
    >(
      user_repository,
      refresh_token_repository,
      password_hasher,
      token_service,
      uuid_generator
    );

    spdlog::debug("AuthController initialized");
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

    auto response_data = nlohmann::json{
      {"access_token", result->access_token},
      {"refresh_token", result->refresh_token},
    };

    callback(ResponseHelper::success(response_data, correlation_id));
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

    nlohmann::json request_body;
    try {
      request_body = nlohmann::json::parse(request->body());
    } catch (
      const nlohmann::json::parse_error& exception
    ) {
      logger->warn(
        "Invalid JSON in refresh request: {}",
        exception.what()
      );

      auto error = Nyx::Core::AppError{
        Nyx::Core::ErrorCode::InvalidJson,
        "Request body must be valid JSON",
        {}
      };

      callback(ResponseHelper::error(error, correlation_id));
      return;
    }

    auto validated = Nyx::Core::RequestValidator::validate(request_body, refresh_request_schema);

    if (!validated.has_value()) {
      logger->warn("Validation failed for refresh request");
      callback(ResponseHelper::error(validated.error(), correlation_id));
      return;
    }

    auto refresh_token = request_body["refresh_token"].get<std::string>();

    auto result = this->auth_service->refresh_access_token(
			refresh_token, logger
		);

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"access_token", result->access_token},
      {"refresh_token", result->refresh_token},
    };

    callback(ResponseHelper::success(response_data, correlation_id));
  }
} // namespace Nyx::Presentation::Http::Auth
