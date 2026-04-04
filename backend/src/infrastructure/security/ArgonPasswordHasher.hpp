#pragma once

#include "application/auth/IPasswordHasher.hpp"

namespace Nyx::Infrastructure::Security {
  class ArgonPasswordHasher
    : public Nyx::Application::Auth::IPasswordHasher {
    public:
      ArgonPasswordHasher();

      auto hash(const std::string& password)
        -> Nyx::Core::Result<std::string> override;

      auto verify(
        const std::string& password,
        const std::string& hash
      ) -> bool override;
  };
} // namespace Nyx::Infrastructure::Security
