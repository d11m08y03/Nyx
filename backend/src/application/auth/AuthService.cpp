#include "application/auth/AuthService.hpp"

namespace Nyx::Application::Auth {
  AuthService::AuthService(
    std::shared_ptr<Nyx::Domain::IUserRepository> user_repository,
    std::shared_ptr<Nyx::Domain::IRefreshTokenRepository> refresh_token_repository,
    std::shared_ptr<IPasswordHasher> password_hasher,
    std::shared_ptr<ITokenService> token_service,
    std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator
  )
    : user_repository(std::move(user_repository))
    , refresh_token_repository(std::move(refresh_token_repository))
    , password_hasher(std::move(password_hasher))
    , token_service(std::move(token_service))
    , uuid_generator(std::move(uuid_generator)) {
    spdlog::debug("AuthService initialized");
  }

  auto AuthService::register_user(
    const RegisterRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<UserProfile> {
    logger->debug(
      "Attempting to register user with email={}",
      request.email
    );

    auto password_hash_result = this->password_hasher->hash(request.password);

    if (!password_hash_result.has_value()) {
      logger->error("Failed to hash password during registration");
      return std::unexpected(password_hash_result.error());
    }

    auto user = Nyx::Domain::User{
      .id = this->uuid_generator->generate(),
      .email = request.email,
      .password_hash = password_hash_result.value(),
    };

    auto create_result = this->user_repository->create(user);

    if (!create_result.has_value()) {
      logger->error("Failed to create user email={}", request.email);
      return std::unexpected(create_result.error());
    }

    auto created_user = create_result.value();

    logger->info(
      "User registered successfully user_id={}, email={}",
      created_user.id, created_user.email
    );

    return UserProfile{
      .id = created_user.id,
      .email = created_user.email,
    };
  }

  auto AuthService::login(
    const LoginRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<TokenPair> {
    logger->debug("Login attempt for email={}", request.email);

    auto user_result = this->user_repository->find_by_email(request.email);

    if (!user_result.has_value()) {
      logger->error("Database error during login for email={}", request.email);
      return std::unexpected(user_result.error());
    }

    if (!user_result->has_value()) {
      logger->warn("Login failed: user not found email={}", request.email);
      return std::unexpected(
        Nyx::Core::AppError::unauthorized("Invalid email or password")
      );
    }

    auto user = user_result->value();

    if (!this->password_hasher->verify(request.password, user.password_hash)) {
      logger->warn("Login failed: invalid password email={}", request.email);
      return std::unexpected(
        Nyx::Core::AppError::unauthorized("Invalid email or password")
      );
    }

    auto token_id = this->uuid_generator->generate();
    auto token_pair = this->token_service->generate_token_pair(
      user.id, user.email, token_id, std::nullopt
    );

    auto store_result = this->store_refresh_token(token_pair, user.id, logger);

    if (!store_result.has_value()) {
      return std::unexpected(store_result.error());
    }

    logger->info("Login successful user_id={}, email={}", user.id, user.email);

    return token_pair;
  }

  auto AuthService::refresh_access_token(
    const std::string& refresh_token,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<TokenPair> {
    logger->debug("Attempting to refresh access token");

    auto claims_result = this->token_service->verify_refresh_token(
      refresh_token
    );

    if (!claims_result.has_value()) {
      logger->warn("Refresh token verification failed");
      return std::unexpected(claims_result.error());
    }

    auto token_hash = this->token_service->hash_token(refresh_token);

    auto stored_token_result = this->refresh_token_repository->find_by_token_hash(
      token_hash
    );

    if (!stored_token_result.has_value()) {
      logger->error("Database error looking up refresh token");
      return std::unexpected(stored_token_result.error());
    }

    if (!stored_token_result->has_value()) {
      logger->warn("Refresh token not found in database");
      return std::unexpected(
        Nyx::Core::AppError::invalid_token("Refresh token is invalid")
      );
    }

    auto stored_token = stored_token_result->value();

    if (stored_token.is_revoked) {
      logger->warn(
        "Reuse detected for token family_id={}, revoking entire family",
        stored_token.family_id
      );

      auto revoke_result = this->refresh_token_repository->revoke_family(
        stored_token.family_id
      );

      if (!revoke_result.has_value()) {
        logger->error("Failed to revoke token family_id={}", stored_token.family_id);
      }

      return std::unexpected(
        Nyx::Core::AppError::invalid_token("Refresh token has been revoked")
      );
    }

    auto revoke_old_result = this->refresh_token_repository->revoke(
      stored_token.id
    );

    if (!revoke_old_result.has_value()) {
      logger->error("Failed to revoke old refresh token id={}", stored_token.id);
      return std::unexpected(revoke_old_result.error());
    }

    auto claims = claims_result.value();
    auto new_token_id = this->uuid_generator->generate();

    auto new_token_pair = this->token_service->generate_token_pair(
      claims.user_id, claims.email, new_token_id, stored_token.family_id
    );

    auto store_result = this->store_refresh_token(
      new_token_pair, claims.user_id, logger
    );

    if (!store_result.has_value()) {
      return std::unexpected(store_result.error());
    }

    logger->info(
      "Token refreshed successfully user_id={}, family_id={}",
      claims.user_id, stored_token.family_id
    );

    return new_token_pair;
  }

  auto AuthService::store_refresh_token(
    const TokenPair& token_pair,
    const std::string& user_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    auto token_hash = this->token_service->hash_token(
      token_pair.refresh_token
    );

    auto refresh_token_entity = Nyx::Domain::RefreshToken{
      .id = token_pair.refresh_token_id,
      .user_id = user_id,
      .token_hash = token_hash,
      .family_id = token_pair.refresh_token_family_id,
      .is_revoked = false,
      .expires_at = token_pair.refresh_token_expires_at,
    };

    auto store_result = this->refresh_token_repository->store(
      refresh_token_entity
    );

    if (!store_result.has_value()) {
      logger->error("Failed to store refresh token for user_id={}", user_id);
      return std::unexpected(store_result.error());
    }

    return {};
  }

  auto AuthService::logout(
    const std::string& refresh_token,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    logger->debug("Attempting to logout");

    auto claims_result = this->token_service->verify_refresh_token(refresh_token);

    if (!claims_result.has_value()) {
      logger->warn("Logout failed: invalid refresh token");
      return std::unexpected(claims_result.error());
    }

    auto token_hash = this->token_service->hash_token(refresh_token);

    auto stored_token_result = this->refresh_token_repository->find_by_token_hash(
      token_hash
    );

    if (!stored_token_result.has_value()) {
      logger->error("Database error during logout");
      return std::unexpected(stored_token_result.error());
    }

    if (!stored_token_result->has_value()) {
      logger->warn("Logout: refresh token not found in database");
      return std::unexpected(
        Nyx::Core::AppError::invalid_token("Refresh token is invalid")
      );
    }

    auto stored_token = stored_token_result->value();

    auto revoke_result = this->refresh_token_repository->revoke_family(
      stored_token.family_id
    );

    if (!revoke_result.has_value()) {
      logger->error("Failed to revoke token family_id={}", stored_token.family_id);
      return std::unexpected(revoke_result.error());
    }

    logger->info(
      "Logout successful user_id={}, family_id={}",
      stored_token.user_id, stored_token.family_id
    );

    return {};
  }
} // namespace Nyx::Application::Auth
