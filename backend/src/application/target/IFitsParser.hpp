#pragma once

#include "core/error/AppError.hpp"
#include "domain/entities/LightCurvePoint.hpp"

#include <string>
#include <vector>

namespace Nyx::Application::Target {
  class IFitsParser {
    public:
      virtual ~IFitsParser() = default;

      virtual auto parse_light_curve(
        const std::string& fits_data
      ) -> Nyx::Core::Result<
        std::vector<Nyx::Domain::LightCurvePoint>
      > = 0;
  };
} // namespace Nyx::Application::Target
