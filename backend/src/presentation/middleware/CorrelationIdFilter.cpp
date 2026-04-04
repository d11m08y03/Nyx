#include "presentation/middleware/CorrelationIdFilter.hpp"

#include "core/logging/Logger.hpp"

#include <drogon/utils/Utilities.h>
#include <spdlog/spdlog.h>

namespace Nyx::Presentation::Middleware {
  auto CorrelationIdFilter::doFilter(
    const drogon::HttpRequestPtr& request,
    drogon::FilterCallback&& /*failure_callback*/,
    drogon::FilterChainCallback&& chain_callback
  ) -> void {
    auto correlation_id =
      std::string(request->getHeader("X-Request-Id"));

    if (correlation_id.empty()) {
      correlation_id = drogon::utils::getUuid();
    }

    request->addHeader(
      "X-Correlation-Id", correlation_id
    );
    request->getAttributes()->insert(
      "correlation_id", correlation_id
    );

    auto request_logger =
      Nyx::Core::Logger::create_request_logger(
        correlation_id
      );
    request->getAttributes()->insert(
      "logger", request_logger
    );

    request_logger->debug(
      "Incoming {} {}",
      request->methodString(),
      request->path()
    );

    chain_callback();
  }
} // namespace Nyx::Presentation::Middleware
