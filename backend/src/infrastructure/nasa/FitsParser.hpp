#pragma once

#include "application/target/IFitsParser.hpp"

namespace Nyx::Infrastructure::Nasa {
  class FitsParser : public Nyx::Application::Target::IFitsParser {
    public:
      auto parse_light_curve(
        const std::string& fits_data
      ) -> Nyx::Core::Result<
        std::vector<Nyx::Domain::LightCurvePoint>
      > override;
  };
} // namespace Nyx::Infrastructure::Nasa
