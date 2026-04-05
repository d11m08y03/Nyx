#pragma once

#include <optional>
#include <string>
#include <vector>

namespace Nyx::Application::Target {
  struct ResolveTargetRequest {
    std::string target_name;
  };

  struct TessObservationResponse {
    std::string id;
    std::string obsid;
    int cadence_seconds;
    double start_time;
    double end_time;
    std::optional<std::string> data_uri;
  };

  struct TargetResponse {
    std::string id;
    std::string canonical_name;
    std::optional<std::string> target_type;
    std::optional<double> right_ascension;
    std::optional<double> declination;
    std::vector<TessObservationResponse> tess_observations;
  };

  struct LightCurvePointResponse {
    double time;
    std::optional<float> pdcsap_flux;
    std::optional<float> pdcsap_flux_err;
    std::optional<float> sap_flux;
    int quality;
  };

  struct LightCurveMetadataResponse {
    std::string tess_observation_id;
    std::string obsid;
    int point_count;
    double time_min;
    double time_max;
  };

  struct LightCurveResponse {
    std::string tess_observation_id;
    std::string obsid;
    int point_count;
    std::vector<LightCurvePointResponse> points;
  };
} // namespace Nyx::Application::Target
