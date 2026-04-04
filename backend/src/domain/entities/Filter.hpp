#pragma once

#include <string>

namespace Nyx::Domain {
  struct Filter {
    std::string id;
    std::string user_id;
    std::string name;
    std::string band;
    double bandwidth_nm;
    std::string brand;
    std::string model;
  };
} // namespace Nyx::Domain
