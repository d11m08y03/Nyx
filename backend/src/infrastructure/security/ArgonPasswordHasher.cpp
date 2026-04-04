#include "infrastructure/security/ArgonPasswordHasher.hpp"

#include <spdlog/spdlog.h>
#include <sodium.h>

namespace Nyx::Infrastructure::Security {
  ArgonPasswordHasher::ArgonPasswordHasher() {
    if (sodium_init() < 0) {
      spdlog::critical("Failed to initialize libsodium");
      throw std::runtime_error(
        "Failed to initialize libsodium"
      );
    }
  }

  auto ArgonPasswordHasher::hash(
    const std::string& password
  ) -> Nyx::Core::Result<std::string> {
    char hashed_password[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(
          hashed_password,
          password.c_str(),
          password.length(),
          crypto_pwhash_OPSLIMIT_MODERATE,
          crypto_pwhash_MEMLIMIT_MODERATE) != 0) {
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Password hashing failed"
        )
      );
    }

    return std::string(hashed_password);
  }

  auto ArgonPasswordHasher::verify(
    const std::string& password,
    const std::string& hash
  ) -> bool {
    return crypto_pwhash_str_verify(
      hash.c_str(),
      password.c_str(),
      password.length()
    ) == 0;
  }
} // namespace Nyx::Infrastructure::Security
