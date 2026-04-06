#pragma once

#include "application/equipment/EquipmentService.hpp"

#include <drogon/HttpController.h>
#include <memory>

namespace Nyx::Presentation::Http::Equipment {
  class CameraController
    : public drogon::HttpController<CameraController> {
    public:
      METHOD_LIST_BEGIN

      ADD_METHOD_TO(
        CameraController::create, "/api/v1/cameras", drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter",
        "Nyx::Presentation::Middleware::CsrfFilter"
      );
      ADD_METHOD_TO(
        CameraController::list, "/api/v1/cameras", drogon::Get,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter"
      );
      ADD_METHOD_TO(
        CameraController::get_by_id, "/api/v1/cameras/{id}", drogon::Get,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter"
      );
      ADD_METHOD_TO(
        CameraController::update, "/api/v1/cameras/{id}", drogon::Put,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter",
        "Nyx::Presentation::Middleware::CsrfFilter"
      );
      ADD_METHOD_TO(
        CameraController::remove, "/api/v1/cameras/{id}", drogon::Delete,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter",
        "Nyx::Presentation::Middleware::CsrfFilter"
      );

      METHOD_LIST_END

      CameraController();

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
      std::shared_ptr<Nyx::Application::Equipment::EquipmentService> equipment_service;
  };
} // namespace Nyx::Presentation::Http::Equipment
