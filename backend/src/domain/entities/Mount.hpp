#pragma once

#include <string>

namespace Nyx::Domain {
  struct Mount {
    std::string id;
    std::string user_id;
    std::string name;
    std::string mount_type;
    bool is_tracked;
    bool has_goto;
    std::string brand;
    std::string model;
  };
} // namespace Nyx::Domain
