#include "infrastructure/storage/LocalFileStorage.hpp"

#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Storage {
  LocalFileStorage::LocalFileStorage(
    const std::string& base_path
  )
    : base_path(base_path) {
    spdlog::debug(
      "LocalFileStorage initialized with base_path={}",
      base_path
    );
  }

  auto LocalFileStorage::save_file(
    const std::string& directory,
    const std::string& filename,
    const char* data,
    std::size_t length
  ) -> Nyx::Core::Result<std::string> {
    auto full_directory = std::filesystem::path(
      this->base_path
    ) / directory;

    try {
      std::filesystem::create_directories(full_directory);
    } catch (const std::filesystem::filesystem_error& e) {
      spdlog::error(
        "Failed to create directory {}: {}",
        full_directory.string(), e.what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to create upload directory"
        )
      );
    }

    auto full_path = full_directory / filename;

    auto output_stream = std::ofstream(
      full_path, std::ios::binary
    );
    if (!output_stream.is_open()) {
      spdlog::error(
        "Failed to open file for writing: {}",
        full_path.string()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to save file"
        )
      );
    }

    output_stream.write(data, static_cast<std::streamsize>(length));
    output_stream.close();

    if (output_stream.fail()) {
      spdlog::error(
        "Failed to write file: {}", full_path.string()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to save file"
        )
      );
    }

    spdlog::debug(
      "Saved file: {} ({} bytes)",
      full_path.string(), length
    );
    return full_path.string();
  }

  auto LocalFileStorage::remove_file(
    const std::string& file_path
  ) -> Nyx::Core::Result<void> {
    try {
      if (std::filesystem::exists(file_path)) {
        std::filesystem::remove(file_path);
        spdlog::debug("Removed file: {}", file_path);
      }
      return {};
    } catch (const std::filesystem::filesystem_error& e) {
      spdlog::error(
        "Failed to remove file {}: {}",
        file_path, e.what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to remove file"
        )
      );
    }
  }

  auto LocalFileStorage::remove_directory(
    const std::string& directory_path
  ) -> Nyx::Core::Result<void> {
    try {
      if (std::filesystem::exists(directory_path)) {
        std::filesystem::remove_all(directory_path);
        spdlog::debug(
          "Removed directory: {}", directory_path
        );
      }
      return {};
    } catch (const std::filesystem::filesystem_error& e) {
      spdlog::error(
        "Failed to remove directory {}: {}",
        directory_path, e.what()
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to remove directory"
        )
      );
    }
  }
} // namespace Nyx::Infrastructure::Storage
