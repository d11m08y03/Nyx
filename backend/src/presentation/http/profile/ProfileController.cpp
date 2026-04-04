#include "presentation/http/profile/ProfileController.hpp"
#include "presentation/http/profile/ProfileSchemas.hpp"
#include "presentation/http/ResponseHelper.hpp"
#include "core/validation/RequestValidator.hpp"
#include "infrastructure/persistence/PostgresUserRepository.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Nyx::Presentation::Http::Profile {
  ProfileController::ProfileController() {
    auto user_repository = std::make_shared<
      Nyx::Infrastructure::Persistence::PostgresUserRepository
    >();

    this->profile_service = std::make_shared<
      Nyx::Application::Profile::ProfileService
    >(user_repository);

    spdlog::debug("ProfileController initialized");
  }

  auto ProfileController::complete_onboarding(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>("correlation_id");
    auto logger =
      request->getAttributes()->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id = request->getAttributes()->get<std::string>("user_id");

    if (!logger) {
      logger = spdlog::default_logger();
    }

    nlohmann::json request_body;
    try {
      request_body = nlohmann::json::parse(request->body());
    } catch (const nlohmann::json::parse_error& exception) {
      logger->warn("Invalid JSON in onboarding request: {}", exception.what());

      auto error = Nyx::Core::AppError{
        Nyx::Core::ErrorCode::InvalidJson,
        "Request body must be valid JSON",
        {}
      };

      callback(ResponseHelper::error(error, correlation_id));
      return;
    }

    auto validated = Nyx::Core::RequestValidator::validate(
      request_body, onboarding_request_schema
    );

    if (!validated.has_value()) {
      logger->warn("Validation failed for onboarding request");
      callback(ResponseHelper::error(validated.error(), correlation_id));
      return;
    }

    auto onboarding_request = Nyx::Application::Profile::OnboardingRequest{
      .display_name = request_body["display_name"].get<std::string>(),
    };

    auto result = this->profile_service->complete_onboarding(
      user_id, onboarding_request, logger
    );

    if (!result.has_value()) {
      callback(ResponseHelper::error(result.error(), correlation_id));
      return;
    }

    auto response_data = nlohmann::json{
      {"display_name", onboarding_request.display_name},
    };

    callback(ResponseHelper::success(response_data, correlation_id));
  }
} // namespace Nyx::Presentation::Http::Profile
