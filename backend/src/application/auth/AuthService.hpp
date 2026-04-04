#pragma once

#include "application/auth/Dtos.hpp"
#include "application/auth/IPasswordHasher.hpp"
#include "application/auth/ITokenService.hpp"
#include "core/error/AppError.hpp"
#include "core/util/UuidGenerator.hpp"
#include "domain/repositories/IRefreshTokenRepository.hpp"
#include "domain/repositories/IUserRepository.hpp"

#include <memory>
#include <spdlog/spdlog.h>

namespace Nyx::Application::Auth {
  class AuthService {
    public:
      AuthService(
        std::shared_ptr<Nyx::Domain::IUserRepository> user_repository,
        std::shared_ptr<Nyx::Domain::IRefreshTokenRepository> refresh_token_repository,
        std::shared_ptr<IPasswordHasher> password_hasher,
        std::shared_ptr<ITokenService> token_service,
        std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator
      );

      auto register_user(
        const RegisterRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<UserProfile>;

      auto login(
        const LoginRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<TokenPair>;

      auto refresh_access_token(
        const std::string& refresh_token,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<TokenPair>;

    private:
      auto store_refresh_token(
        const TokenPair& token_pair,
        const std::string& user_id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

      std::shared_ptr<Nyx::Domain::IUserRepository> user_repository;
      std::shared_ptr<Nyx::Domain::IRefreshTokenRepository> refresh_token_repository;
      std::shared_ptr<IPasswordHasher> password_hasher;
      std::shared_ptr<ITokenService> token_service;
      std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator;
  };
} // namespace Nyx::Application::Auth
