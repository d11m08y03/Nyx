#include "presentation/middleware/JwtAuthFilter.hpp"

#include "infrastructure/config/EnvironmentConfig.hpp"
#include "infrastructure/security/JwtTokenService.hpp"
#include "presentation/http/ResponseHelper.hpp"

#include <spdlog/spdlog.h>

namespace Nyx::Presentation::Middleware {
  JwtAuthFilter::JwtAuthFilter() {
    auto config = std::make_shared<
      Nyx::Infrastructure::Config::EnvironmentConfig
    >();

    this->token_service = std::make_shared<
      Nyx::Infrastructure::Security::JwtTokenService
    >(config);
  }

  auto JwtAuthFilter::doFilter(
    const drogon::HttpRequestPtr& request,
    drogon::FilterCallback&& failure_callback,
    drogon::FilterChainCallback&& chain_callback
  ) -> void {
    auto logger =
      request->getAttributes()
        ->get<std::shared_ptr<spdlog::logger>>(
          "logger"
        );

    if (!logger) {
      logger = spdlog::default_logger();
    }

    auto correlation_id =
      request->getAttributes()
        ->get<std::string>("correlation_id");

    auto authorization_header =
      std::string(
        request->getHeader("Authorization")
      );

    if (authorization_header.empty()
        || authorization_header.substr(0, 7)
             != "Bearer ") {
      logger->warn(
        "Missing or malformed Authorization header"
      );

      auto error =
        Nyx::Core::AppError::unauthorized(
          "Bearer token is required"
        );
      failure_callback(
        Nyx::Presentation::Http::ResponseHelper
          ::error(error, correlation_id)
      );
      return;
    }

    auto token = authorization_header.substr(7);
    auto claims_result =
      this->token_service->verify_access_token(
        token
      );

    if (!claims_result.has_value()) {
      logger->warn("JWT verification failed");
      failure_callback(
        Nyx::Presentation::Http::ResponseHelper
          ::error(
            claims_result.error(), correlation_id
          )
      );
      return;
    }

    auto claims = claims_result.value();
    request->getAttributes()->insert(
      "user_id", claims.user_id
    );
    request->getAttributes()->insert(
      "user_email", claims.email
    );

    logger->debug(
      "Authenticated user_id={}, email={}",
      claims.user_id, claims.email
    );

    chain_callback();
  }
} // namespace Nyx::Presentation::Middleware
