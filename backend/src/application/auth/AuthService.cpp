#include "application/auth/AuthService.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace Nyx::Application::Auth {
  AuthService::AuthService(
    std::shared_ptr<Nyx::Domain::IUserRepository> user_repository,
    std::shared_ptr<Nyx::Domain::IRefreshTokenRepository> refresh_token_repository,
    std::shared_ptr<IPasswordHasher> password_hasher,
    std::shared_ptr<ITokenService> token_service,
    std::shared_ptr<Nyx::Core::IUuidGenerator> uuid_generator,
    std::shared_ptr<Nyx::Domain::IVerificationTokenRepository> verification_token_repository,
    std::shared_ptr<IEmailSender> email_sender,
    std::shared_ptr<IGoogleAuthClient> google_auth_client
  )
    : user_repository(std::move(user_repository))
    , refresh_token_repository(std::move(refresh_token_repository))
    , password_hasher(std::move(password_hasher))
    , token_service(std::move(token_service))
    , uuid_generator(std::move(uuid_generator))
    , verification_token_repository(std::move(verification_token_repository))
    , email_sender(std::move(email_sender))
    , google_auth_client(std::move(google_auth_client)) {
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

    auto password_hash_result =
      this->password_hasher->hash(request.password);

    if (!password_hash_result.has_value()) {
      logger->error(
        "Failed to hash password during registration"
      );
      return std::unexpected(
        password_hash_result.error()
      );
    }

    auto user = Nyx::Domain::User{
      .id = this->uuid_generator->generate(),
      .email = request.email,
      .password_hash = password_hash_result.value(),
      .display_name = std::nullopt,
      .email_verified = false,
      .auth_provider = "local",
      .google_id = std::nullopt,
    };

    auto create_result =
      this->user_repository->create(user);

    if (!create_result.has_value()) {
      logger->error(
        "Failed to create user email={}",
        request.email
      );
      return std::unexpected(create_result.error());
    }

    auto created_user = create_result.value();

    auto send_result = this->send_verification_token(
      created_user.id, created_user.email, logger
    );

    if (!send_result.has_value()) {
      logger->warn(
        "Failed to send verification email for "
        "user_id={}, but user was created",
        created_user.id
      );
    }

    logger->info(
      "User registered successfully user_id={}, "
      "email={}",
      created_user.id, created_user.email
    );

    return UserProfile{
      .id = created_user.id,
      .email = created_user.email,
      .email_verified = false,
    };
  }

  auto AuthService::login(
    const LoginRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<TokenPair> {
    logger->debug(
      "Login attempt for email={}", request.email
    );

    auto user_result =
      this->user_repository->find_by_email(
        request.email
      );

    if (!user_result.has_value()) {
      logger->error(
        "Database error during login for email={}",
        request.email
      );
      return std::unexpected(user_result.error());
    }

    if (!user_result->has_value()) {
      logger->warn(
        "Login failed: user not found email={}",
        request.email
      );
      return std::unexpected(
        Nyx::Core::AppError::unauthorized(
          "Invalid email or password"
        )
      );
    }

    auto user = user_result->value();

    if (user.auth_provider == "google"
      && !user.password_hash.has_value()) {
      logger->warn(
        "Login failed: Google-only account email={}",
        request.email
      );
      return std::unexpected(
        Nyx::Core::AppError::unauthorized(
          "This account uses Google sign-in. "
          "Please use Google to log in."
        )
      );
    }

    if (!user.password_hash.has_value()
      || !this->password_hasher->verify(
          request.password,
          user.password_hash.value()
        )) {
      logger->warn(
        "Login failed: invalid password email={}",
        request.email
      );
      return std::unexpected(
        Nyx::Core::AppError::unauthorized(
          "Invalid email or password"
        )
      );
    }

    if (!user.email_verified) {
      logger->warn(
        "Login failed: email not verified email={}",
        request.email
      );
      return std::unexpected(
        Nyx::Core::AppError::email_not_verified()
      );
    }

    auto token_id = this->uuid_generator->generate();
    auto token_pair =
      this->token_service->generate_token_pair(
        user.id, user.email,
        token_id, std::nullopt
      );

    auto store_result = this->store_refresh_token(
      token_pair, user.id, logger
    );

    if (!store_result.has_value()) {
      return std::unexpected(store_result.error());
    }

    logger->info(
      "Login successful user_id={}, email={}",
      user.id, user.email
    );

    return token_pair;
  }

  auto AuthService::verify_email(
    const std::string& token,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    logger->debug("Attempting to verify email");

    auto token_hash =
      this->token_service->hash_token(token);

    auto token_result =
      this->verification_token_repository
        ->find_by_token_hash(token_hash);

    if (!token_result.has_value()) {
      logger->error(
        "Database error looking up verification token"
      );
      return std::unexpected(token_result.error());
    }

    if (!token_result->has_value()) {
      logger->warn("Verification token not found");
      return std::unexpected(
        Nyx::Core::AppError::invalid_token(
          "Verification token is invalid"
        )
      );
    }

    auto stored_token = token_result->value();

    if (stored_token.used_at.has_value()) {
      logger->warn(
        "Verification token already used id={}",
        stored_token.id
      );
      return std::unexpected(
        Nyx::Core::AppError::invalid_token(
          "Verification token has already been used"
        )
      );
    }

    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto now_stream = std::ostringstream{};
    now_stream << std::put_time(
      std::gmtime(&now_time), "%Y-%m-%dT%H:%M:%SZ"
    );

    if (now_stream.str() > stored_token.expires_at) {
      logger->warn(
        "Verification token expired id={}",
        stored_token.id
      );
      return std::unexpected(
        Nyx::Core::AppError::invalid_token(
          "Verification token has expired"
        )
      );
    }

    auto mark_result =
      this->verification_token_repository->mark_used(
        stored_token.id
      );

    if (!mark_result.has_value()) {
      logger->error(
        "Failed to mark verification token as used"
      );
      return std::unexpected(mark_result.error());
    }

    auto verify_result =
      this->user_repository->verify_email(
        stored_token.user_id
      );

    if (!verify_result.has_value()) {
      logger->error(
        "Failed to verify email for user_id={}",
        stored_token.user_id
      );
      return std::unexpected(verify_result.error());
    }

    logger->info(
      "Email verified for user_id={}",
      stored_token.user_id
    );

    return {};
  }

  auto AuthService::resend_verification(
    const std::string& email,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    logger->debug(
      "Resend verification request for email={}",
      email
    );

    auto user_result =
      this->user_repository->find_by_email(email);

    if (!user_result.has_value()) {
      logger->error(
        "Database error during resend verification"
      );
      return {};
    }

    if (!user_result->has_value()) {
      logger->debug(
        "Resend verification: user not found "
        "(returning success to avoid leaking)"
      );
      return {};
    }

    auto user = user_result->value();

    if (user.email_verified) {
      logger->debug(
        "Resend verification: already verified"
      );
      return {};
    }

    auto revoke_result =
      this->verification_token_repository
        ->revoke_all_for_user(user.id);

    if (!revoke_result.has_value()) {
      logger->warn(
        "Failed to revoke old verification tokens "
        "for user_id={}",
        user.id
      );
    }

    return this->send_verification_token(
      user.id, user.email, logger
    );
  }

  auto AuthService::google_login(
    const GoogleLoginRequest& request,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<TokenPair> {
    logger->debug("Google login attempt");

    auto google_result =
      this->google_auth_client->exchange_code(
        request.code, request.redirect_uri
      );

    if (!google_result.has_value()) {
      logger->error("Google code exchange failed");
      return std::unexpected(google_result.error());
    }

    auto google_user = google_result.value();

    logger->debug(
      "Google user info: email={}, google_id={}",
      google_user.email, google_user.google_id
    );

    auto existing_google_user =
      this->user_repository->find_by_google_id(
        google_user.google_id
      );

    if (!existing_google_user.has_value()) {
      return std::unexpected(
        existing_google_user.error()
      );
    }

    if (existing_google_user->has_value()) {
      auto user = existing_google_user->value();
      logger->info(
        "Google login: existing user user_id={}",
        user.id
      );

      auto token_id =
        this->uuid_generator->generate();
      auto token_pair =
        this->token_service->generate_token_pair(
          user.id, user.email,
          token_id, std::nullopt
        );

      auto store_result = this->store_refresh_token(
        token_pair, user.id, logger
      );

      if (!store_result.has_value()) {
        return std::unexpected(store_result.error());
      }

      return token_pair;
    }

    auto existing_email_user =
      this->user_repository->find_by_email(
        google_user.email
      );

    if (!existing_email_user.has_value()) {
      return std::unexpected(
        existing_email_user.error()
      );
    }

    if (existing_email_user->has_value()) {
      logger->warn(
        "Google login: email conflict with "
        "local account email={}",
        google_user.email
      );
      return std::unexpected(
        Nyx::Core::AppError::conflict(
          "An account with this email already "
          "exists. Please log in with your password."
        )
      );
    }

    auto new_user = Nyx::Domain::User{
      .id = this->uuid_generator->generate(),
      .email = google_user.email,
      .password_hash = std::nullopt,
      .display_name = google_user.name,
      .email_verified = true,
      .auth_provider = "google",
      .google_id = google_user.google_id,
    };

    auto create_result =
      this->user_repository->create(new_user);

    if (!create_result.has_value()) {
      logger->error(
        "Failed to create Google user email={}",
        google_user.email
      );
      return std::unexpected(create_result.error());
    }

    auto created_user = create_result.value();

    logger->info(
      "Google login: new user created user_id={}, "
      "email={}",
      created_user.id, created_user.email
    );

    auto token_id = this->uuid_generator->generate();
    auto token_pair =
      this->token_service->generate_token_pair(
        created_user.id, created_user.email,
        token_id, std::nullopt
      );

    auto store_result = this->store_refresh_token(
      token_pair, created_user.id, logger
    );

    if (!store_result.has_value()) {
      return std::unexpected(store_result.error());
    }

    return token_pair;
  }

  auto AuthService::refresh_access_token(
    const std::string& refresh_token,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<TokenPair> {
    logger->debug("Attempting to refresh access token");

    auto claims_result =
      this->token_service->verify_refresh_token(
        refresh_token
      );

    if (!claims_result.has_value()) {
      logger->warn("Refresh token verification failed");
      return std::unexpected(claims_result.error());
    }

    auto token_hash =
      this->token_service->hash_token(refresh_token);

    auto stored_token_result =
      this->refresh_token_repository
        ->find_by_token_hash(token_hash);

    if (!stored_token_result.has_value()) {
      logger->error(
        "Database error looking up refresh token"
      );
      return std::unexpected(
        stored_token_result.error()
      );
    }

    if (!stored_token_result->has_value()) {
      logger->warn(
        "Refresh token not found in database"
      );
      return std::unexpected(
        Nyx::Core::AppError::invalid_token(
          "Refresh token is invalid"
        )
      );
    }

    auto stored_token =
      stored_token_result->value();

    if (stored_token.is_revoked) {
      logger->warn(
        "Reuse detected for token family_id={}, "
        "revoking entire family",
        stored_token.family_id
      );

      auto revoke_result =
        this->refresh_token_repository->revoke_family(
          stored_token.family_id
        );

      if (!revoke_result.has_value()) {
        logger->error(
          "Failed to revoke token family_id={}",
          stored_token.family_id
        );
      }

      return std::unexpected(
        Nyx::Core::AppError::invalid_token(
          "Refresh token has been revoked"
        )
      );
    }

    auto revoke_old_result =
      this->refresh_token_repository->revoke(
        stored_token.id
      );

    if (!revoke_old_result.has_value()) {
      logger->error(
        "Failed to revoke old refresh token id={}",
        stored_token.id
      );
      return std::unexpected(
        revoke_old_result.error()
      );
    }

    auto claims = claims_result.value();
    auto new_token_id =
      this->uuid_generator->generate();

    auto new_token_pair =
      this->token_service->generate_token_pair(
        claims.user_id, claims.email,
        new_token_id, stored_token.family_id
      );

    auto store_result = this->store_refresh_token(
      new_token_pair, claims.user_id, logger
    );

    if (!store_result.has_value()) {
      return std::unexpected(store_result.error());
    }

    logger->info(
      "Token refreshed successfully user_id={}, "
      "family_id={}",
      claims.user_id, stored_token.family_id
    );

    return new_token_pair;
  }

  auto AuthService::store_refresh_token(
    const TokenPair& token_pair,
    const std::string& user_id,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    auto token_hash =
      this->token_service->hash_token(
        token_pair.refresh_token
      );

    auto refresh_token_entity =
      Nyx::Domain::RefreshToken{
        .id = token_pair.refresh_token_id,
        .user_id = user_id,
        .token_hash = token_hash,
        .family_id =
          token_pair.refresh_token_family_id,
        .is_revoked = false,
        .expires_at =
          token_pair.refresh_token_expires_at,
      };

    auto store_result =
      this->refresh_token_repository->store(
        refresh_token_entity
      );

    if (!store_result.has_value()) {
      logger->error(
        "Failed to store refresh token for "
        "user_id={}",
        user_id
      );
      return std::unexpected(store_result.error());
    }

    return {};
  }

  auto AuthService::send_verification_token(
    const std::string& user_id,
    const std::string& email,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    auto raw_token = this->uuid_generator->generate();
    auto token_hash =
      this->token_service->hash_token(raw_token);

    auto now = std::chrono::system_clock::now();
    auto expiry =
      now + std::chrono::hours(24);
    auto expiry_time =
      std::chrono::system_clock::to_time_t(expiry);
    auto expiry_stream = std::ostringstream{};
    expiry_stream << std::put_time(
      std::gmtime(&expiry_time),
      "%Y-%m-%dT%H:%M:%SZ"
    );

    auto verification_token =
      Nyx::Domain::VerificationToken{
        .id = this->uuid_generator->generate(),
        .user_id = user_id,
        .token_hash = token_hash,
        .expires_at = expiry_stream.str(),
        .used_at = std::nullopt,
      };

    auto store_result =
      this->verification_token_repository->store(
        verification_token
      );

    if (!store_result.has_value()) {
      logger->error(
        "Failed to store verification token for "
        "user_id={}",
        user_id
      );
      return std::unexpected(store_result.error());
    }

    auto send_result =
      this->email_sender->send_verification_email(
        email, raw_token
      );

    if (!send_result.has_value()) {
      logger->error(
        "Failed to send verification email for "
        "user_id={}",
        user_id
      );
      return std::unexpected(send_result.error());
    }

    logger->debug(
      "Verification email sent for user_id={}",
      user_id
    );

    return {};
  }

  auto AuthService::logout(
    const std::string& refresh_token,
    std::shared_ptr<spdlog::logger> logger
  ) -> Nyx::Core::Result<void> {
    logger->debug("Attempting to logout");

    auto claims_result =
      this->token_service->verify_refresh_token(
        refresh_token
      );

    if (!claims_result.has_value()) {
      logger->warn(
        "Logout failed: invalid refresh token"
      );
      return std::unexpected(claims_result.error());
    }

    auto token_hash =
      this->token_service->hash_token(refresh_token);

    auto stored_token_result =
      this->refresh_token_repository
        ->find_by_token_hash(token_hash);

    if (!stored_token_result.has_value()) {
      logger->error("Database error during logout");
      return std::unexpected(
        stored_token_result.error()
      );
    }

    if (!stored_token_result->has_value()) {
      logger->warn(
        "Logout: refresh token not found in database"
      );
      return std::unexpected(
        Nyx::Core::AppError::invalid_token(
          "Refresh token is invalid"
        )
      );
    }

    auto stored_token =
      stored_token_result->value();

    auto revoke_result =
      this->refresh_token_repository->revoke_family(
        stored_token.family_id
      );

    if (!revoke_result.has_value()) {
      logger->error(
        "Failed to revoke token family_id={}",
        stored_token.family_id
      );
      return std::unexpected(revoke_result.error());
    }

    logger->info(
      "Logout successful user_id={}, family_id={}",
      stored_token.user_id, stored_token.family_id
    );

    return {};
  }
} // namespace Nyx::Application::Auth
