#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <string>

namespace Nyx::Core {
  class Logger {
    public:
      static auto initialize(
        const std::string& log_level
      ) -> void;

      static auto create_request_logger(
        const std::string& correlation_id
      ) -> std::shared_ptr<spdlog::logger>;
  };
} // namespace Nyx::Core
