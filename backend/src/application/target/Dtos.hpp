#pragma once

#include "domain/entities/LightCurvePoint.hpp"

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
    std::vector<Nyx::Domain::LightCurvePoint> points;
  };

  struct LightCurveJsonResponse {
    std::string tess_observation_id;
    std::string obsid;
    int point_count;
    std::string points_json;
  };
  struct UserObservationPoint {
    double time;
    double relative_flux;
    double relative_flux_error;
    std::string session_id;
    std::string captured_at;
  };

  struct TessComparisonData {
    std::string tess_observation_id;
    std::string obsid;
    int cadence_seconds;
    int point_count;
    std::vector<Nyx::Domain::LightCurvePoint> points;
  };

  struct UserObservationData {
    int session_count;
    int point_count;
    std::vector<UserObservationPoint> points;
  };

  struct LightCurveComparisonResponse {
    TargetResponse target;
    std::string time_system;
    std::string time_system_note;
    TessComparisonData tess;
    UserObservationData user_observations;
  };
} // namespace Nyx::Application::Target
