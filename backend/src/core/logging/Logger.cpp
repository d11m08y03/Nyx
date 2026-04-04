#include "core/logging/Logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace Nyx::Core {
  auto Logger::initialize(
    const std::string& log_level
  ) -> void {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto default_logger = std::make_shared<spdlog::logger>("nyx", console_sink);

    default_logger->set_pattern(
      "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v"
    );
    default_logger->set_level(
      spdlog::level::from_str(log_level)
    );
    default_logger->flush_on(spdlog::level::warn);

    spdlog::set_default_logger(default_logger);
    spdlog::debug(
      "Logger initialized with level: {}", log_level
    );
  }

  auto Logger::create_request_logger(
    const std::string& correlation_id
  ) -> std::shared_ptr<spdlog::logger> {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto request_logger = std::make_shared<spdlog::logger>(
			correlation_id, console_sink
		);

    request_logger->set_pattern(
      "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] ["
        + correlation_id + "] %v"
    );
    request_logger->set_level(spdlog::get_level());

    return request_logger;
  }
} // namespace Nyx::Core
