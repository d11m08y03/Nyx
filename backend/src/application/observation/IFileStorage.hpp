#pragma once

#include "core/error/AppError.hpp"

#include <cstddef>
#include <string>

namespace Nyx::Application::Observation {
  class IFileStorage {
    public:
      virtual ~IFileStorage() = default;

      virtual auto save_file(
        const std::string& directory,
        const std::string& filename,
        const char* data,
        std::size_t length
      ) -> Nyx::Core::Result<std::string> = 0;

      virtual auto remove_file(
        const std::string& file_path
      ) -> Nyx::Core::Result<void> = 0;

      virtual auto remove_directory(
        const std::string& directory_path
      ) -> Nyx::Core::Result<void> = 0;
  };
} // namespace Nyx::Application::Observation
