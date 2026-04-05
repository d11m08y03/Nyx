#pragma once

#include "core/error/AppError.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Nyx::Application::Observation {
  struct DecodedImage {
    std::vector<uint16_t> pixels;
    int width;
    int height;
    int channels;
  };

  class IDngDecoder {
    public:
      virtual ~IDngDecoder() = default;

      virtual auto decode(
        const std::string& file_path
      ) -> Nyx::Core::Result<DecodedImage> = 0;
  };
} // namespace Nyx::Application::Observation
