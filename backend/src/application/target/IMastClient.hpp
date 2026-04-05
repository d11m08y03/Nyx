#pragma once

#include "core/error/AppError.hpp"

#include <optional>
#include <string>
#include <vector>

namespace Nyx::Application::Target {
  struct ResolvedTarget {
    std::string canonical_name;
    std::optional<std::string> target_type;
    std::optional<double> right_ascension;
    std::optional<double> declination;
  };

  struct MastObservation {
    std::string obsid;
    int cadence_seconds;
    double start_time;
    double end_time;
  };

  struct DataProduct {
    std::string data_uri;
    std::string description;
    std::string product_type;
    std::string filename;
  };

  class IMastClient {
    public:
      virtual ~IMastClient() = default;

      virtual auto resolve_target(
        const std::string& name
      ) -> Nyx::Core::Result<ResolvedTarget> = 0;

      virtual auto search_tess_timeseries(
        double ra, double dec, double radius = 0.005
      ) -> Nyx::Core::Result<std::vector<MastObservation>> = 0;

      virtual auto get_data_products(
        const std::string& obsid
      ) -> Nyx::Core::Result<std::vector<DataProduct>> = 0;

      virtual auto download_fits(
        const std::string& data_uri
      ) -> Nyx::Core::Result<std::string> = 0;
  };
} // namespace Nyx::Application::Target
