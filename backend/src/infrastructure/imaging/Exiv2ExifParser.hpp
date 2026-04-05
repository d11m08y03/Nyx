#pragma once

#include "application/observation/IExifParser.hpp"

namespace Nyx::Infrastructure::Imaging {
  class Exiv2ExifParser
    : public Nyx::Application::Observation::IExifParser {
    public:
      auto parse(
        const std::string& file_path
      ) -> Nyx::Core::Result<
        Nyx::Application::Observation::ExifData
      > override;
  };
} // namespace Nyx::Infrastructure::Imaging
