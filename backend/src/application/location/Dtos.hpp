#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace Nyx::Application::Location {
  struct CreateLocationRequest {
    std::string name;
    double latitude;
    double longitude;
    std::optional<int16_t> bortle_class;
  };

  struct UpdateLocationRequest {
    std::string name;
    double latitude;
    double longitude;
    std::optional<int16_t> bortle_class;
  };

  struct LocationResponse {
    std::string id;
    std::string user_id;
    std::string name;
    double latitude;
    double longitude;
    std::optional<int16_t> bortle_class;
  };
} // namespace Nyx::Application::Location
