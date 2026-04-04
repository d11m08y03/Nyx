#pragma once

#include "core/error/AppError.hpp"

#include <string>

namespace Nyx::Application::Auth {
  class IPasswordHasher {
    public:
      virtual ~IPasswordHasher() = default;

      virtual auto hash(
        const std::string& password
      ) -> Nyx::Core::Result<std::string> = 0;

      virtual auto verify(
        const std::string& password,
        const std::string& hash
      ) -> bool = 0;
  };
} // namespace Nyx::Application::Auth
