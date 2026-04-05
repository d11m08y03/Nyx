#pragma once

#include "application/observation/IFileStorage.hpp"

#include <string>

namespace Nyx::Infrastructure::Storage {
  class LocalFileStorage
    : public Nyx::Application::Observation::IFileStorage {
    public:
      explicit LocalFileStorage(
        const std::string& base_path
      );

      auto save_file(
        const std::string& directory,
        const std::string& filename,
        const char* data,
        std::size_t length
      ) -> Nyx::Core::Result<std::string> override;

      auto remove_file(
        const std::string& file_path
      ) -> Nyx::Core::Result<void> override;

      auto remove_directory(
        const std::string& directory_path
      ) -> Nyx::Core::Result<void> override;

    private:
      std::string base_path;
  };
} // namespace Nyx::Infrastructure::Storage
