#pragma once

#include "application/location/LocationService.hpp"

#include <drogon/HttpController.h>
#include <memory>

namespace Nyx::Presentation::Http::Location {
  class LocationController
    : public drogon::HttpController<LocationController> {
    public:
      METHOD_LIST_BEGIN

      ADD_METHOD_TO(
        LocationController::create,
        "/api/v1/observing-locations",
        drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter"
      );
      ADD_METHOD_TO(
        LocationController::list,
        "/api/v1/observing-locations",
        drogon::Get,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter"
      );
      ADD_METHOD_TO(
        LocationController::get_by_id,
        "/api/v1/observing-locations/{id}",
        drogon::Get,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter"
      );
      ADD_METHOD_TO(
        LocationController::update,
        "/api/v1/observing-locations/{id}",
        drogon::Put,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter"
      );
      ADD_METHOD_TO(
        LocationController::remove,
        "/api/v1/observing-locations/{id}",
        drogon::Delete,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter"
      );

      METHOD_LIST_END

      LocationController();

      auto create(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
      ) -> void;

      auto list(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
      ) -> void;

      auto get_by_id(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& id
      ) -> void;

      auto update(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& id
      ) -> void;

      auto remove(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& id
      ) -> void;

    private:
      std::shared_ptr<Nyx::Application::Location::LocationService> location_service;
  };
} // namespace Nyx::Presentation::Http::Location
