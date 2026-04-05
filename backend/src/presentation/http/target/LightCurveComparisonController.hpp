#pragma once

#include "application/target/LightCurveComparisonService.hpp"

#include <drogon/HttpController.h>
#include <memory>

namespace Nyx::Presentation::Http::Target {
  class LightCurveComparisonController
    : public drogon::HttpController<
        LightCurveComparisonController
      > {
    public:
      METHOD_LIST_BEGIN

      ADD_METHOD_TO(
        LightCurveComparisonController::get_comparison,
        "/api/v1/targets/{1}/light-curve-comparison",
        drogon::Get,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter"
      );

      METHOD_LIST_END

      LightCurveComparisonController();

      auto get_comparison(
        const drogon::HttpRequestPtr& request,
        std::function<void(
          const drogon::HttpResponsePtr&
        )>&& callback,
        const std::string& target_id
      ) -> void;

    private:
      std::shared_ptr<
        Nyx::Application::Target::
          LightCurveComparisonService
      > comparison_service;
  };
} // namespace Nyx::Presentation::Http::Target
