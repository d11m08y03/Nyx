#include "application/auth/AuthService.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace Nyx::Application::Auth::Tests {
  class MockUserRepository
    : public Nyx::Domain::IUserRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::User>, create,
        (const Nyx::Domain::User& user), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::User>>),
        find_by_email,
        (const std::string& email), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, update_display_name,
        (const std::string& user_id,
         const std::string& display_name),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::User>>),
        find_by_id,
        (const std::string& id), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, verify_email,
        (const std::string& user_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::User>>),
        find_by_google_id,
        (const std::string& google_id), (override)
      );
  };

  class MockRefreshTokenRepository
    : public Nyx::Domain::IRefreshTokenRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<void>, store,
        (const Nyx::Domain::RefreshToken& token),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<
          std::optional<Nyx::Domain::RefreshToken>
        >),
        find_by_token_hash,
        (const std::string& hash), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, revoke,
        (const std::string& id), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, revoke_family,
        (const std::string& family_id), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, revoke_all_for_user,
        (const std::string& user_id), (override)
      );
  };

  class MockPasswordHasher : public IPasswordHasher {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<std::string>, hash,
        (const std::string& password), (override)
      );
      MOCK_METHOD(
        bool, verify,
        (const std::string& password,
         const std::string& hash),
        (override)
      );
  };

  class MockTokenService : public ITokenService {
    public:
      MOCK_METHOD(
        TokenPair, generate_token_pair,
        (const std::string& user_id,
         const std::string& email,
         const std::string& token_id,
         const std::optional<std::string>& family_id),
        (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<TokenClaims>,
        verify_refresh_token,
        (const std::string& token), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<TokenClaims>,
        verify_access_token,
        (const std::string& token), (override)
      );
      MOCK_METHOD(
        std::string, hash_token,
        (const std::string& token), (override)
      );
  };

  class MockUuidGenerator
    : public Nyx::Core::IUuidGenerator {
    public:
      MOCK_METHOD(
        std::string, generate, (), (override)
      );
  };

  class MockVerificationTokenRepository
    : public Nyx::Domain::IVerificationTokenRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<void>, store,
        (const Nyx::Domain::VerificationToken& token),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<
          std::optional<Nyx::Domain::VerificationToken>
        >),
        find_by_token_hash,
        (const std::string& token_hash), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, mark_used,
        (const std::string& id), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, revoke_all_for_user,
        (const std::string& user_id), (override)
      );
  };

  class MockEmailSender : public IEmailSender {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<void>,
        send_verification_email,
        (const std::string& to_email,
         const std::string& verification_token),
        (override)
      );
  };

  class MockGoogleAuthClient
    : public IGoogleAuthClient {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<GoogleUserInfo>,
        exchange_code,
        (const std::string& code,
         const std::string& redirect_uri),
        (override)
      );
  };

  class AuthServiceTest : public ::testing::Test {
    protected:
      auto SetUp() -> void override {
        this->user_repo =
          std::make_shared<MockUserRepository>();
        this->refresh_token_repo =
          std::make_shared<MockRefreshTokenRepository>();
        this->password_hasher =
          std::make_shared<MockPasswordHasher>();
        this->token_service =
          std::make_shared<MockTokenService>();
        this->uuid_gen =
          std::make_shared<MockUuidGenerator>();
        this->verification_token_repo =
          std::make_shared<MockVerificationTokenRepository>();
        this->email_sender =
          std::make_shared<MockEmailSender>();
        this->google_auth_client =
          std::make_shared<MockGoogleAuthClient>();
        this->logger = spdlog::default_logger();

        this->service =
          std::make_unique<AuthService>(
            this->user_repo,
            this->refresh_token_repo,
            this->password_hasher,
            this->token_service,
            this->uuid_gen,
            this->verification_token_repo,
            this->email_sender,
            this->google_auth_client
          );
      }

      std::shared_ptr<MockUserRepository> user_repo;
      std::shared_ptr<MockRefreshTokenRepository>
        refresh_token_repo;
      std::shared_ptr<MockPasswordHasher>
        password_hasher;
      std::shared_ptr<MockTokenService> token_service;
      std::shared_ptr<MockUuidGenerator> uuid_gen;
      std::shared_ptr<MockVerificationTokenRepository>
        verification_token_repo;
      std::shared_ptr<MockEmailSender> email_sender;
      std::shared_ptr<MockGoogleAuthClient>
        google_auth_client;
      std::shared_ptr<spdlog::logger> logger;
      std::unique_ptr<AuthService> service;
  };

  // Registration

  TEST_F(AuthServiceTest, RegisterUserSuccess) {
    auto request = RegisterRequest{
      .email = "a@b.com", .password = "secret123"
    };

    EXPECT_CALL(*this->uuid_gen, generate())
      .WillRepeatedly(::testing::Return("uuid-1"));
    EXPECT_CALL(*this->password_hasher, hash("secret123"))
      .WillOnce(::testing::Return(std::string("hashed")));
    EXPECT_CALL(*this->user_repo, create(::testing::_))
      .WillOnce(::testing::Return(Nyx::Domain::User{
        .id = "uuid-1",
        .email = "a@b.com",
        .password_hash = "hashed",
      }));
    EXPECT_CALL(*this->token_service, hash_token(::testing::_))
      .WillOnce(::testing::Return("hashed-token"));
    EXPECT_CALL(*this->verification_token_repo, store(::testing::_))
      .WillOnce(::testing::Return(Nyx::Core::Result<void>{}));
    EXPECT_CALL(*this->email_sender, send_verification_email("a@b.com", ::testing::_))
      .WillOnce(::testing::Return(Nyx::Core::Result<void>{}));

    auto result = this->service->register_user(
      request, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "uuid-1");
    EXPECT_EQ(result->email, "a@b.com");
    EXPECT_FALSE(result->email_verified);
  }

  TEST_F(AuthServiceTest, RegisterUserDuplicateEmail) {
    auto request = RegisterRequest{
      .email = "a@b.com", .password = "secret123"
    };

    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("uuid-1"));
    EXPECT_CALL(*this->password_hasher, hash(::testing::_))
      .WillOnce(::testing::Return(std::string("hashed")));
    EXPECT_CALL(*this->user_repo, create(::testing::_))
      .WillOnce(::testing::Return(std::unexpected(
        Nyx::Core::AppError::conflict(
          "Email already exists"
        )
      )));

    auto result = this->service->register_user(
      request, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::Conflict
    );
  }

  TEST_F(AuthServiceTest, RegisterUserHashFailure) {
    auto request = RegisterRequest{
      .email = "a@b.com", .password = "secret123"
    };

    EXPECT_CALL(*this->password_hasher, hash(::testing::_))
      .WillOnce(::testing::Return(std::unexpected(
        Nyx::Core::AppError::internal("Hash failed")
      )));

    auto result = this->service->register_user(
      request, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::InternalError
    );
  }

  TEST_F(AuthServiceTest, RegisterSendsVerificationEmail) {
    auto request = RegisterRequest{
      .email = "test@example.com",
      .password = "secret123"
    };

    EXPECT_CALL(*this->uuid_gen, generate())
      .WillRepeatedly(::testing::Return("uuid-1"));
    EXPECT_CALL(*this->password_hasher, hash(::testing::_))
      .WillOnce(::testing::Return(std::string("hashed")));
    EXPECT_CALL(*this->user_repo, create(::testing::_))
      .WillOnce(::testing::Return(Nyx::Domain::User{
        .id = "uuid-1",
        .email = "test@example.com",
        .password_hash = "hashed",
      }));
    EXPECT_CALL(*this->token_service, hash_token(::testing::_))
      .WillOnce(::testing::Return("hashed-vtoken"));
    EXPECT_CALL(*this->verification_token_repo, store(::testing::_))
      .WillOnce(::testing::Return(Nyx::Core::Result<void>{}));
    EXPECT_CALL(*this->email_sender, send_verification_email(
      "test@example.com", ::testing::_
    )).WillOnce(::testing::Return(Nyx::Core::Result<void>{}));

    auto result = this->service->register_user(
      request, this->logger
    );
    ASSERT_TRUE(result.has_value());
  }

  // Login

  TEST_F(AuthServiceTest, LoginSuccess) {
    auto request = LoginRequest{
      .email = "a@b.com", .password = "secret123"
    };
    auto user = Nyx::Domain::User{
      .id = "user-1",
      .email = "a@b.com",
      .password_hash = "hashed",
      .email_verified = true,
    };
    auto token_pair = TokenPair{
      .access_token = "access",
      .refresh_token = "refresh",
      .refresh_token_id = "tid-1",
      .refresh_token_family_id = "fid-1",
      .refresh_token_expires_at = "2026-04-11T00:00:00Z",
    };

    EXPECT_CALL(*this->user_repo, find_by_email("a@b.com"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::User>(user)
      ));
    EXPECT_CALL(*this->password_hasher, verify("secret123", "hashed"))
      .WillOnce(::testing::Return(true));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("tid-1"));
    EXPECT_CALL(*this->token_service, generate_token_pair(
      "user-1", "a@b.com", "tid-1", ::testing::_
    )).WillOnce(::testing::Return(token_pair));
    EXPECT_CALL(*this->token_service, hash_token("refresh"))
      .WillOnce(::testing::Return("hashed-refresh"));
    EXPECT_CALL(*this->refresh_token_repo, store(::testing::_))
      .WillOnce(::testing::Return(
        Nyx::Core::Result<void>{}
      ));

    auto result =
      this->service->login(request, this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->access_token, "access");
    EXPECT_EQ(result->refresh_token, "refresh");
  }

  TEST_F(AuthServiceTest, LoginUserNotFound) {
    auto request = LoginRequest{
      .email = "a@b.com", .password = "secret123"
    };

    EXPECT_CALL(*this->user_repo, find_by_email("a@b.com"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::User>(std::nullopt)
      ));

    auto result =
      this->service->login(request, this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::AuthenticationRequired
    );
  }

  TEST_F(AuthServiceTest, LoginWrongPassword) {
    auto request = LoginRequest{
      .email = "a@b.com", .password = "wrong"
    };
    auto user = Nyx::Domain::User{
      .id = "user-1",
      .email = "a@b.com",
      .password_hash = "hashed",
      .email_verified = true,
    };

    EXPECT_CALL(*this->user_repo, find_by_email("a@b.com"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::User>(user)
      ));
    EXPECT_CALL(*this->password_hasher, verify("wrong", "hashed"))
      .WillOnce(::testing::Return(false));

    auto result =
      this->service->login(request, this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::AuthenticationRequired
    );
  }

  TEST_F(AuthServiceTest, LoginFailsForUnverifiedUser) {
    auto request = LoginRequest{
      .email = "a@b.com", .password = "secret123"
    };
    auto user = Nyx::Domain::User{
      .id = "user-1",
      .email = "a@b.com",
      .password_hash = "hashed",
      .email_verified = false,
    };

    EXPECT_CALL(*this->user_repo, find_by_email("a@b.com"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::User>(user)
      ));
    EXPECT_CALL(*this->password_hasher, verify("secret123", "hashed"))
      .WillOnce(::testing::Return(true));

    auto result =
      this->service->login(request, this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::EmailNotVerified
    );
  }

  TEST_F(AuthServiceTest, LoginRejectsGoogleOnlyUser) {
    auto request = LoginRequest{
      .email = "a@b.com", .password = "secret123"
    };
    auto user = Nyx::Domain::User{
      .id = "user-1",
      .email = "a@b.com",
      .password_hash = std::nullopt,
      .email_verified = true,
      .auth_provider = "google",
      .google_id = "google-123",
    };

    EXPECT_CALL(*this->user_repo, find_by_email("a@b.com"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::User>(user)
      ));

    auto result =
      this->service->login(request, this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::AuthenticationRequired
    );
  }

  // Email verification

  TEST_F(AuthServiceTest, VerifyEmailSuccess) {
    auto stored_token = Nyx::Domain::VerificationToken{
      .id = "vt-1",
      .user_id = "user-1",
      .token_hash = "hashed-vtoken",
      .expires_at = "2099-01-01T00:00:00Z",
      .used_at = std::nullopt,
    };

    EXPECT_CALL(*this->token_service, hash_token("raw-token"))
      .WillOnce(::testing::Return("hashed-vtoken"));
    EXPECT_CALL(*this->verification_token_repo, find_by_token_hash("hashed-vtoken"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::VerificationToken>(
          stored_token
        )
      ));
    EXPECT_CALL(*this->verification_token_repo, mark_used("vt-1"))
      .WillOnce(::testing::Return(
        Nyx::Core::Result<void>{}
      ));
    EXPECT_CALL(*this->user_repo, verify_email("user-1"))
      .WillOnce(::testing::Return(
        Nyx::Core::Result<void>{}
      ));

    auto result = this->service->verify_email(
      "raw-token", this->logger
    );

    ASSERT_TRUE(result.has_value());
  }

  TEST_F(AuthServiceTest, VerifyEmailTokenNotFound) {
    EXPECT_CALL(*this->token_service, hash_token("bad"))
      .WillOnce(::testing::Return("hashed-bad"));
    EXPECT_CALL(*this->verification_token_repo, find_by_token_hash("hashed-bad"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::VerificationToken>(
          std::nullopt
        )
      ));

    auto result = this->service->verify_email(
      "bad", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::InvalidToken
    );
  }

  TEST_F(AuthServiceTest, VerifyEmailTokenAlreadyUsed) {
    auto stored_token = Nyx::Domain::VerificationToken{
      .id = "vt-1",
      .user_id = "user-1",
      .token_hash = "h",
      .expires_at = "2099-01-01T00:00:00Z",
      .used_at = "2026-01-01T00:00:00Z",
    };

    EXPECT_CALL(*this->token_service, hash_token("tok"))
      .WillOnce(::testing::Return("h"));
    EXPECT_CALL(*this->verification_token_repo, find_by_token_hash("h"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::VerificationToken>(
          stored_token
        )
      ));

    auto result = this->service->verify_email(
      "tok", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::InvalidToken
    );
  }

  TEST_F(AuthServiceTest, VerifyEmailTokenExpired) {
    auto stored_token = Nyx::Domain::VerificationToken{
      .id = "vt-1",
      .user_id = "user-1",
      .token_hash = "h",
      .expires_at = "2020-01-01T00:00:00Z",
      .used_at = std::nullopt,
    };

    EXPECT_CALL(*this->token_service, hash_token("tok"))
      .WillOnce(::testing::Return("h"));
    EXPECT_CALL(*this->verification_token_repo, find_by_token_hash("h"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::VerificationToken>(
          stored_token
        )
      ));

    auto result = this->service->verify_email(
      "tok", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::InvalidToken
    );
  }

  // Resend verification

  TEST_F(AuthServiceTest, ResendVerificationSuccess) {
    auto user = Nyx::Domain::User{
      .id = "user-1",
      .email = "a@b.com",
      .password_hash = "hashed",
      .email_verified = false,
    };

    EXPECT_CALL(*this->user_repo, find_by_email("a@b.com"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::User>(user)
      ));
    EXPECT_CALL(*this->verification_token_repo, revoke_all_for_user("user-1"))
      .WillOnce(::testing::Return(
        Nyx::Core::Result<void>{}
      ));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillRepeatedly(::testing::Return("uuid-1"));
    EXPECT_CALL(*this->token_service, hash_token(::testing::_))
      .WillOnce(::testing::Return("hashed-new"));
    EXPECT_CALL(*this->verification_token_repo, store(::testing::_))
      .WillOnce(::testing::Return(
        Nyx::Core::Result<void>{}
      ));
    EXPECT_CALL(*this->email_sender, send_verification_email("a@b.com", ::testing::_))
      .WillOnce(::testing::Return(
        Nyx::Core::Result<void>{}
      ));

    auto result = this->service->resend_verification(
      "a@b.com", this->logger
    );

    ASSERT_TRUE(result.has_value());
  }

  TEST_F(AuthServiceTest, ResendVerificationUserNotFoundReturnsSuccess) {
    EXPECT_CALL(*this->user_repo, find_by_email("notfound@b.com"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::User>(std::nullopt)
      ));

    auto result = this->service->resend_verification(
      "notfound@b.com", this->logger
    );

    ASSERT_TRUE(result.has_value());
  }

  // Google OAuth

  TEST_F(AuthServiceTest, GoogleLoginNewUserCreatesAccount) {
    auto google_info = GoogleUserInfo{
      .google_id = "google-123",
      .email = "g@example.com",
      .email_verified = true,
      .name = "Google User",
    };

    EXPECT_CALL(*this->google_auth_client, exchange_code("code123", "http://localhost"))
      .WillOnce(::testing::Return(google_info));
    EXPECT_CALL(*this->user_repo, find_by_google_id("google-123"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::User>(std::nullopt)
      ));
    EXPECT_CALL(*this->user_repo, find_by_email("g@example.com"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::User>(std::nullopt)
      ));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillRepeatedly(::testing::Return("uuid-g1"));
    EXPECT_CALL(*this->user_repo, create(::testing::_))
      .WillOnce(::testing::Return(Nyx::Domain::User{
        .id = "uuid-g1",
        .email = "g@example.com",
        .password_hash = std::nullopt,
        .email_verified = true,
        .auth_provider = "google",
        .google_id = "google-123",
      }));
    EXPECT_CALL(*this->token_service, generate_token_pair(
      "uuid-g1", "g@example.com", ::testing::_, ::testing::_
    )).WillOnce(::testing::Return(TokenPair{
      .access_token = "g-access",
      .refresh_token = "g-refresh",
      .refresh_token_id = "tid-g",
      .refresh_token_family_id = "fid-g",
      .refresh_token_expires_at = "2026-04-11",
    }));
    EXPECT_CALL(*this->token_service, hash_token("g-refresh"))
      .WillOnce(::testing::Return("hashed-g"));
    EXPECT_CALL(*this->refresh_token_repo, store(::testing::_))
      .WillOnce(::testing::Return(
        Nyx::Core::Result<void>{}
      ));

    auto result = this->service->google_login(
      GoogleLoginRequest{
        .code = "code123",
        .redirect_uri = "http://localhost"
      },
      this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->access_token, "g-access");
  }

  TEST_F(AuthServiceTest, GoogleLoginExistingGoogleUser) {
    auto existing_user = Nyx::Domain::User{
      .id = "user-g",
      .email = "g@example.com",
      .password_hash = std::nullopt,
      .email_verified = true,
      .auth_provider = "google",
      .google_id = "google-123",
    };

    EXPECT_CALL(*this->google_auth_client, exchange_code("code", "http://x"))
      .WillOnce(::testing::Return(GoogleUserInfo{
        .google_id = "google-123",
        .email = "g@example.com",
        .email_verified = true,
        .name = std::nullopt,
      }));
    EXPECT_CALL(*this->user_repo, find_by_google_id("google-123"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::User>(existing_user)
      ));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("tid-1"));
    EXPECT_CALL(*this->token_service, generate_token_pair(
      "user-g", "g@example.com", "tid-1", ::testing::_
    )).WillOnce(::testing::Return(TokenPair{
      .access_token = "access",
      .refresh_token = "refresh",
      .refresh_token_id = "tid-1",
      .refresh_token_family_id = "fid-1",
      .refresh_token_expires_at = "2026-04-11",
    }));
    EXPECT_CALL(*this->token_service, hash_token("refresh"))
      .WillOnce(::testing::Return("h"));
    EXPECT_CALL(*this->refresh_token_repo, store(::testing::_))
      .WillOnce(::testing::Return(
        Nyx::Core::Result<void>{}
      ));

    auto result = this->service->google_login(
      GoogleLoginRequest{
        .code = "code", .redirect_uri = "http://x"
      },
      this->logger
    );

    ASSERT_TRUE(result.has_value());
  }

  TEST_F(AuthServiceTest, GoogleLoginEmailConflictWithLocalUser) {
    auto local_user = Nyx::Domain::User{
      .id = "user-local",
      .email = "conflict@b.com",
      .password_hash = "hashed",
      .email_verified = true,
      .auth_provider = "local",
    };

    EXPECT_CALL(*this->google_auth_client, exchange_code("code", "http://x"))
      .WillOnce(::testing::Return(GoogleUserInfo{
        .google_id = "g-new",
        .email = "conflict@b.com",
        .email_verified = true,
        .name = std::nullopt,
      }));
    EXPECT_CALL(*this->user_repo, find_by_google_id("g-new"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::User>(std::nullopt)
      ));
    EXPECT_CALL(*this->user_repo, find_by_email("conflict@b.com"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::User>(local_user)
      ));

    auto result = this->service->google_login(
      GoogleLoginRequest{
        .code = "code", .redirect_uri = "http://x"
      },
      this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::Conflict
    );
  }

  TEST_F(AuthServiceTest, GoogleLoginCodeExchangeFailure) {
    EXPECT_CALL(*this->google_auth_client, exchange_code("bad", "http://x"))
      .WillOnce(::testing::Return(std::unexpected(
        Nyx::Core::AppError{
          Nyx::Core::ErrorCode::ExternalServiceError,
          "Google auth failed", {}
        }
      )));

    auto result = this->service->google_login(
      GoogleLoginRequest{
        .code = "bad", .redirect_uri = "http://x"
      },
      this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::ExternalServiceError
    );
  }

  // Refresh token

  TEST_F(AuthServiceTest, RefreshSuccess) {
    auto stored = Nyx::Domain::RefreshToken{
      .id = "tid-1",
      .user_id = "user-1",
      .token_hash = "hashed-tok",
      .family_id = "fid-1",
      .is_revoked = false,
      .expires_at = "2026-04-11T00:00:00Z",
    };
    auto new_pair = TokenPair{
      .access_token = "new-access",
      .refresh_token = "new-refresh",
      .refresh_token_id = "tid-2",
      .refresh_token_family_id = "fid-1",
      .refresh_token_expires_at = "2026-04-11T00:00:00Z",
    };

    EXPECT_CALL(*this->token_service, verify_refresh_token("old-refresh"))
      .WillOnce(::testing::Return(TokenClaims{
        .user_id = "user-1", .email = "a@b.com"
      }));
    EXPECT_CALL(*this->token_service, hash_token("old-refresh"))
      .WillOnce(::testing::Return("hashed-tok"));
    EXPECT_CALL(*this->refresh_token_repo, find_by_token_hash("hashed-tok"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::RefreshToken>(stored)
      ));
    EXPECT_CALL(*this->refresh_token_repo, revoke("tid-1"))
      .WillOnce(::testing::Return(
        Nyx::Core::Result<void>{}
      ));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("tid-2"));
    EXPECT_CALL(*this->token_service, generate_token_pair(
      "user-1", "a@b.com", "tid-2",
      ::testing::Optional(std::string("fid-1"))
    )).WillOnce(::testing::Return(new_pair));
    EXPECT_CALL(*this->token_service, hash_token("new-refresh"))
      .WillOnce(::testing::Return("hashed-new"));
    EXPECT_CALL(*this->refresh_token_repo, store(::testing::_))
      .WillOnce(::testing::Return(
        Nyx::Core::Result<void>{}
      ));

    auto result =
      this->service->refresh_access_token(
        "old-refresh", this->logger
      );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->access_token, "new-access");
    EXPECT_EQ(result->refresh_token, "new-refresh");
  }

  TEST_F(AuthServiceTest, RefreshTokenNotInDatabase) {
    EXPECT_CALL(*this->token_service, verify_refresh_token("tok"))
      .WillOnce(::testing::Return(TokenClaims{
        .user_id = "u1", .email = "a@b.com"
      }));
    EXPECT_CALL(*this->token_service, hash_token("tok"))
      .WillOnce(::testing::Return("h"));
    EXPECT_CALL(*this->refresh_token_repo, find_by_token_hash("h"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::RefreshToken>(
          std::nullopt
        )
      ));

    auto result =
      this->service->refresh_access_token(
        "tok", this->logger
      );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::InvalidToken
    );
  }

  TEST_F(AuthServiceTest, RefreshReuseDetectionRevokesFamily) {
    auto revoked_token = Nyx::Domain::RefreshToken{
      .id = "tid-1",
      .user_id = "user-1",
      .token_hash = "h",
      .family_id = "fid-1",
      .is_revoked = true,
      .expires_at = "2026-04-11T00:00:00Z",
    };

    EXPECT_CALL(*this->token_service, verify_refresh_token("tok"))
      .WillOnce(::testing::Return(TokenClaims{
        .user_id = "u1", .email = "a@b.com"
      }));
    EXPECT_CALL(*this->token_service, hash_token("tok"))
      .WillOnce(::testing::Return("h"));
    EXPECT_CALL(*this->refresh_token_repo, find_by_token_hash("h"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::RefreshToken>(
          revoked_token
        )
      ));
    EXPECT_CALL(*this->refresh_token_repo, revoke_family("fid-1"))
      .WillOnce(::testing::Return(
        Nyx::Core::Result<void>{}
      ));

    auto result =
      this->service->refresh_access_token(
        "tok", this->logger
      );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::InvalidToken
    );
  }

  // Logout

  TEST_F(AuthServiceTest, LogoutSuccess) {
    auto stored = Nyx::Domain::RefreshToken{
      .id = "tid-1",
      .user_id = "user-1",
      .token_hash = "h",
      .family_id = "fid-1",
      .is_revoked = false,
      .expires_at = "2026-04-11T00:00:00Z",
    };

    EXPECT_CALL(*this->token_service, verify_refresh_token("tok"))
      .WillOnce(::testing::Return(TokenClaims{
        .user_id = "u1", .email = "a@b.com"
      }));
    EXPECT_CALL(*this->token_service, hash_token("tok"))
      .WillOnce(::testing::Return("h"));
    EXPECT_CALL(*this->refresh_token_repo, find_by_token_hash("h"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::RefreshToken>(stored)
      ));
    EXPECT_CALL(*this->refresh_token_repo, revoke_family("fid-1"))
      .WillOnce(::testing::Return(
        Nyx::Core::Result<void>{}
      ));

    auto result =
      this->service->logout("tok", this->logger);

    ASSERT_TRUE(result.has_value());
  }

  TEST_F(AuthServiceTest, LogoutInvalidToken) {
    EXPECT_CALL(*this->token_service, verify_refresh_token("bad"))
      .WillOnce(::testing::Return(std::unexpected(
        Nyx::Core::AppError::invalid_token("Invalid")
      )));

    auto result =
      this->service->logout("bad", this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::InvalidToken
    );
  }

  TEST_F(AuthServiceTest, LogoutTokenNotInDatabase) {
    EXPECT_CALL(*this->token_service, verify_refresh_token("tok"))
      .WillOnce(::testing::Return(TokenClaims{
        .user_id = "u1", .email = "a@b.com"
      }));
    EXPECT_CALL(*this->token_service, hash_token("tok"))
      .WillOnce(::testing::Return("h"));
    EXPECT_CALL(*this->refresh_token_repo, find_by_token_hash("h"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::RefreshToken>(
          std::nullopt
        )
      ));

    auto result =
      this->service->logout("tok", this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::InvalidToken
    );
  }
} // namespace Nyx::Application::Auth::Tests
