#pragma once

#include "application/auth/Dtos.hpp"
#include "application/auth/IEmailSender.hpp"
#include "application/auth/IGoogleAuthClient.hpp"
#include "application/auth/IPasswordHasher.hpp"
#include "application/auth/ITokenService.hpp"
#include "core/error/AppError.hpp"
#include "core/util/UuidGenerator.hpp"
#include "domain/repositories/IRefreshTokenRepository.hpp"
#include "domain/repositories/IUserRepository.hpp"
#include "domain/repositories/IVerificationTokenRepository.hpp"

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
        std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator,
        std::shared_ptr<Nyx::Domain::IVerificationTokenRepository> verification_token_repository,
        std::shared_ptr<IEmailSender> email_sender,
        std::shared_ptr<IGoogleAuthClient> google_auth_client
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

      auto logout(
        const std::string& refresh_token,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

      auto verify_email(
        const std::string& token,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

      auto resend_verification(
        const std::string& email,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

      auto google_login(
        const GoogleLoginRequest& request,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<TokenPair>;

    private:
      auto store_refresh_token(
        const TokenPair& token_pair,
        const std::string& user_id,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

      auto send_verification_token(
        const std::string& user_id,
        const std::string& email,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<void>;

      std::shared_ptr<Nyx::Domain::IUserRepository> user_repository;
      std::shared_ptr<Nyx::Domain::IRefreshTokenRepository> refresh_token_repository;
      std::shared_ptr<IPasswordHasher> password_hasher;
      std::shared_ptr<ITokenService> token_service;
      std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator;
      std::shared_ptr<Nyx::Domain::IVerificationTokenRepository> verification_token_repository;
      std::shared_ptr<IEmailSender> email_sender;
      std::shared_ptr<IGoogleAuthClient> google_auth_client;
  };
} // namespace Nyx::Application::Auth
