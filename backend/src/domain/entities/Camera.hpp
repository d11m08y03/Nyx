#pragma once

#include <string>

namespace Nyx::Domain {
  struct Camera {
    std::string id;
    std::string user_id;
    std::string name;
    std::string sensor_type;
    std::string brand;
    std::string model;
    double pixel_size_um;
    std::string resolution;
  };
} // namespace Nyx::Domain
