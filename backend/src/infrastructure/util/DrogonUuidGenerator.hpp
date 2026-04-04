#pragma once

#include "core/util/UuidGenerator.hpp"

#include <drogon/utils/Utilities.h>

namespace Nyx::Infrastructure::Util {
  class DrogonUuidGenerator
    : public Nyx::Core::IUuidGenerator {
    public:
      auto generate() -> std::string override {
        return drogon::utils::getUuid();
      }
  };
} // namespace Nyx::Infrastructure::Util
