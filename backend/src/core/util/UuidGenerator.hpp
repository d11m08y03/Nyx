#pragma once

#include <string>

namespace Nyx::Core {
  class IUuidGenerator {
    public:
      virtual ~IUuidGenerator() = default;

      virtual auto generate() -> std::string = 0;
  };
} // namespace Nyx::Core
