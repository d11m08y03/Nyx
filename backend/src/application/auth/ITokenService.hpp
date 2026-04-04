#pragma once

#include "core/error/AppError.hpp"

#include <optional>
#include <string>

namespace Nyx::Application::Auth {
  struct TokenClaims {
    std::string user_id;
    std::string email;
  };

  struct TokenPair {
    std::string access_token;
    std::string refresh_token;
    std::string refresh_token_id;
    std::string refresh_token_family_id;
    std::string refresh_token_expires_at;
  };

  class ITokenService {
    public:
      virtual ~ITokenService() = default;

      virtual auto generate_token_pair(
        const std::string& user_id,
        const std::string& email,
        const std::string& token_id,
        const std::optional<std::string>& family_id
      ) -> TokenPair = 0;

      virtual auto verify_refresh_token(
        const std::string& token
      ) -> Nyx::Core::Result<TokenClaims> = 0;

      virtual auto verify_access_token(
        const std::string& token
      ) -> Nyx::Core::Result<TokenClaims> = 0;

      virtual auto hash_token(
        const std::string& token
      ) -> std::string = 0;
  };
} // namespace Nyx::Application::Auth
