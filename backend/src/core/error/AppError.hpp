#pragma once

#include <expected>
#include <string>
#include <vector>

namespace Nyx::Core {
	enum class ErrorCode {
		ValidationError,
		InvalidJson,
		AuthenticationRequired,
		InvalidToken,
		TokenExpired,
		PermissionDenied,
		ResourceNotFound,
		Conflict,
		RateLimitExceeded,
		InternalError,
		DatabaseError,
		ExternalServiceError,
		EmailNotVerified,
	};

	struct FieldError {
		std::string field;
		std::string message;
	};

	struct AppError {
		ErrorCode code;
		std::string message;
		std::vector<FieldError> details;

		static auto validation(
			std::string message,
			std::vector<FieldError> details = {}
		) -> AppError {
			return AppError{
				ErrorCode::ValidationError,
					std::move(message),
					std::move(details)
			};
		}

		static auto not_found(std::string message) -> AppError {
			return AppError{ErrorCode::ResourceNotFound, std::move(message), {}};
		}

		static auto unauthorized(std::string message) -> AppError {
			return AppError{
				ErrorCode::AuthenticationRequired, std::move(message), {}};
		}

		static auto forbidden(std::string message) -> AppError {
			return AppError{ErrorCode::PermissionDenied, std::move(message), {}};
		}

		static auto internal(std::string message) -> AppError {
			return AppError{ErrorCode::InternalError, std::move(message), {}};
		}

		static auto conflict(std::string message) -> AppError {
			return AppError{ErrorCode::Conflict, std::move(message), {}};
		}

		static auto invalid_token(std::string message) -> AppError {
			return AppError{ErrorCode::InvalidToken, std::move(message), {}};
		}

		static auto token_expired() -> AppError {
			return AppError{ErrorCode::TokenExpired, "Token has expired", {}};
		}

		static auto email_not_verified() -> AppError {
			return AppError{
				ErrorCode::EmailNotVerified,
				"Email address has not been verified",
				{}
			};
		}

		[[nodiscard]] auto http_status_code() const -> int {
			switch (code) {
				case ErrorCode::ValidationError:
				case ErrorCode::InvalidJson:
					return 400;
				case ErrorCode::AuthenticationRequired:
				case ErrorCode::InvalidToken:
				case ErrorCode::TokenExpired:
					return 401;
				case ErrorCode::PermissionDenied:
				case ErrorCode::EmailNotVerified:
					return 403;
				case ErrorCode::ResourceNotFound:
					return 404;
				case ErrorCode::Conflict:
					return 409;
				case ErrorCode::RateLimitExceeded:
					return 429;
				case ErrorCode::InternalError:
				case ErrorCode::DatabaseError:
				case ErrorCode::ExternalServiceError:
					return 500;
			}
			return 500;
		}

		[[nodiscard]] auto error_code_string() const -> std::string {
			switch (code) {
				case ErrorCode::ValidationError:
					return "VALIDATION_ERROR";
				case ErrorCode::InvalidJson:
					return "INVALID_JSON";
				case ErrorCode::AuthenticationRequired:
					return "AUTHENTICATION_REQUIRED";
				case ErrorCode::InvalidToken:
					return "INVALID_TOKEN";
				case ErrorCode::TokenExpired:
					return "TOKEN_EXPIRED";
				case ErrorCode::PermissionDenied:
					return "PERMISSION_DENIED";
				case ErrorCode::ResourceNotFound:
					return "RESOURCE_NOT_FOUND";
				case ErrorCode::Conflict:
					return "CONFLICT";
				case ErrorCode::RateLimitExceeded:
					return "RATE_LIMIT_EXCEEDED";
				case ErrorCode::InternalError:
					return "INTERNAL_ERROR";
				case ErrorCode::DatabaseError:
					return "DATABASE_ERROR";
				case ErrorCode::ExternalServiceError:
					return "EXTERNAL_SERVICE_ERROR";
				case ErrorCode::EmailNotVerified:
					return "EMAIL_NOT_VERIFIED";
			}
			return "UNKNOWN_ERROR";
		}
	};

	template<typename T> using Result = std::expected<T, AppError>;
} // namespace Nyx::Core
