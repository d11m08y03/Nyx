#pragma once

#include "infrastructure/config/EnvironmentConfig.hpp"

namespace Nyx::Infrastructure::Database {
  class DatabaseManager {
    public:
      static auto initialize(
        const Nyx::Infrastructure::Config::
          EnvironmentConfig& config
      ) -> void;

      static auto check_health() -> void;
  };
} // namespace Nyx::Infrastructure::Database
