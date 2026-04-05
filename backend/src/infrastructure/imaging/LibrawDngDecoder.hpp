#pragma once

#include "application/observation/IDngDecoder.hpp"

namespace Nyx::Infrastructure::Imaging {
  class LibrawDngDecoder
    : public Nyx::Application::Observation::IDngDecoder {
    public:
      auto decode(
        const std::string& file_path
      ) -> Nyx::Core::Result<
        Nyx::Application::Observation::DecodedImage
      > override;
  };
} // namespace Nyx::Infrastructure::Imaging
