#pragma once

#include <string>

namespace Nyx::Domain {
  struct Telescope {
    std::string id;
    std::string user_id;
    std::string name;
    int aperture_mm;
    int focal_length_mm;
    std::string optical_design;
    std::string brand;
    std::string model;
  };
} // namespace Nyx::Domain
