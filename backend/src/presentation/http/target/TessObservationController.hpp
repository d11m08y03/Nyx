#pragma once

#include "application/target/TargetService.hpp"

#include <drogon/HttpController.h>
#include <memory>

namespace Nyx::Presentation::Http::Target {
  class TessObservationController
    : public drogon::HttpController<TessObservationController> {
    public:
      METHOD_LIST_BEGIN

      ADD_METHOD_TO(
        TessObservationController::discover_products,
        "/api/v1/tess-observations/{1}/discover-products",
        drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter"
      );
      ADD_METHOD_TO(
        TessObservationController::fetch_light_curve,
        "/api/v1/tess-observations/{1}/fetch-light-curve",
        drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter"
      );
      ADD_METHOD_TO(
        TessObservationController::get_light_curve,
        "/api/v1/tess-observations/{1}/light-curve",
        drogon::Get,
        "Nyx::Presentation::Middleware::CorrelationIdFilter"
      );

      METHOD_LIST_END

      TessObservationController();

      auto discover_products(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& id
      ) -> void;

      auto fetch_light_curve(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& id
      ) -> void;

      auto get_light_curve(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& id
      ) -> void;

    private:
      std::shared_ptr<
        Nyx::Application::Target::TargetService
      > target_service;
  };
} // namespace Nyx::Presentation::Http::Target
