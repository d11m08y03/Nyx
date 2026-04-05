#include "presentation/http/target/LightCurveComparisonController.hpp"
#include "presentation/http/ResponseHelper.hpp"
#include "core/validation/RequestValidator.hpp"
#include "infrastructure/persistence/PostgresLightCurvePointRepository.hpp"
#include "infrastructure/persistence/PostgresObservationImageRepository.hpp"
#include "infrastructure/persistence/PostgresObservationSessionRepository.hpp"
#include "infrastructure/persistence/PostgresTargetRepository.hpp"
#include "infrastructure/persistence/PostgresTessObservationRepository.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Nyx::Presentation::Http::Target {
  LightCurveComparisonController::
    LightCurveComparisonController() {
    this->comparison_service = std::make_shared<
      Nyx::Application::Target::
        LightCurveComparisonService
    >(
      std::make_shared<Nyx::Infrastructure::Persistence::
        PostgresTargetRepository>(),
      std::make_shared<Nyx::Infrastructure::Persistence::
        PostgresTessObservationRepository>(),
      std::make_shared<Nyx::Infrastructure::Persistence::
        PostgresLightCurvePointRepository>(),
      std::make_shared<Nyx::Infrastructure::Persistence::
        PostgresObservationSessionRepository>(),
      std::make_shared<Nyx::Infrastructure::Persistence::
        PostgresObservationImageRepository>()
    );

    spdlog::debug(
      "LightCurveComparisonController initialized"
    );
  }

  auto LightCurveComparisonController::get_comparison(
    const drogon::HttpRequestPtr& request,
    std::function<void(
      const drogon::HttpResponsePtr&
    )>&& callback,
    const std::string& target_id
  ) -> void {
    auto correlation_id =
      request->getAttributes()->get<std::string>(
        "correlation_id"
      );
    auto logger =
      request->getAttributes()
        ->get<std::shared_ptr<spdlog::logger>>("logger");
    auto user_id =
      request->getAttributes()->get<std::string>(
        "user_id"
      );

    if (!logger) logger = spdlog::default_logger();

    if (!Nyx::Core::RequestValidator::is_valid_uuid(
          target_id
        )) {
      callback(ResponseHelper::error(
        Nyx::Core::AppError::validation(
          "Invalid UUID format",
          {{"target_id", "must be a valid UUID"}}
        ),
        correlation_id
      ));
      return;
    }

    auto tess_observation_id =
      request->getParameter("tess_observation_id");
    if (tess_observation_id.empty()) {
      callback(ResponseHelper::error(
        Nyx::Core::AppError::validation(
          "Missing required parameter",
          {{"tess_observation_id",
            "is required as a query parameter"}}
        ),
        correlation_id
      ));
      return;
    }

    if (!Nyx::Core::RequestValidator::is_valid_uuid(
          tess_observation_id
        )) {
      callback(ResponseHelper::error(
        Nyx::Core::AppError::validation(
          "Invalid UUID format",
          {{"tess_observation_id",
            "must be a valid UUID"}}
        ),
        correlation_id
      ));
      return;
    }

    auto quality_filter_param =
      request->getParameter("quality_filter");
    auto quality_filter =
      quality_filter_param == "true"
      || quality_filter_param == "1";

    auto result =
      this->comparison_service->get_comparison(
        user_id, target_id, tess_observation_id,
        quality_filter, logger
      );

    if (!result.has_value()) {
      callback(ResponseHelper::error(
        result.error(), correlation_id
      ));
      return;
    }

    auto tess_points_array = nlohmann::json::array();
    for (const auto& point : result->tess.points) {
      auto point_json = nlohmann::json{
        {"time", point.time},
        {"quality", point.quality},
      };
      point_json["pdcsap_flux"] =
        point.pdcsap_flux.has_value()
          ? nlohmann::json(point.pdcsap_flux.value())
          : nlohmann::json(nullptr);
      point_json["pdcsap_flux_err"] =
        point.pdcsap_flux_err.has_value()
          ? nlohmann::json(
              point.pdcsap_flux_err.value()
            )
          : nlohmann::json(nullptr);
      point_json["sap_flux"] =
        point.sap_flux.has_value()
          ? nlohmann::json(point.sap_flux.value())
          : nlohmann::json(nullptr);
      tess_points_array.push_back(point_json);
    }

    auto user_points_array = nlohmann::json::array();
    for (const auto& point :
         result->user_observations.points) {
      user_points_array.push_back(nlohmann::json{
        {"time", point.time},
        {"relative_flux", point.relative_flux},
        {"relative_flux_error",
         point.relative_flux_error},
        {"session_id", point.session_id},
        {"captured_at", point.captured_at},
      });
    }

    auto target_json = nlohmann::json{
      {"id", result->target.id},
      {"canonical_name",
       result->target.canonical_name},
    };
    target_json["right_ascension"] =
      result->target.right_ascension.has_value()
        ? nlohmann::json(
            result->target.right_ascension.value()
          )
        : nlohmann::json(nullptr);
    target_json["declination"] =
      result->target.declination.has_value()
        ? nlohmann::json(
            result->target.declination.value()
          )
        : nlohmann::json(nullptr);

    auto response_json = nlohmann::json{
      {"target", target_json},
      {"time_system", result->time_system},
      {"time_system_note", result->time_system_note},
      {"tess", {
        {"tess_observation_id",
         result->tess.tess_observation_id},
        {"obsid", result->tess.obsid},
        {"cadence_seconds",
         result->tess.cadence_seconds},
        {"point_count", result->tess.point_count},
        {"points", tess_points_array},
      }},
      {"user_observations", {
        {"session_count",
         result->user_observations.session_count},
        {"point_count",
         result->user_observations.point_count},
        {"points", user_points_array},
      }},
    };

    callback(ResponseHelper::success(
      response_json, correlation_id
    ));
  }
} // namespace Nyx::Presentation::Http::Target
