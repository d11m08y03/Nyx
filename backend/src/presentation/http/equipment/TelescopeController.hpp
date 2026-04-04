#pragma once

#include "application/equipment/EquipmentService.hpp"

#include <drogon/HttpController.h>
#include <memory>

namespace Nyx::Presentation::Http::Equipment {
  class TelescopeController
    : public drogon::HttpController<TelescopeController> {
    public:
      METHOD_LIST_BEGIN

      ADD_METHOD_TO(
        TelescopeController::create,
        "/api/v1/telescopes",
        drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter"
      );
      ADD_METHOD_TO(
        TelescopeController::list,
        "/api/v1/telescopes",
        drogon::Get,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter"
      );
      ADD_METHOD_TO(
        TelescopeController::get_by_id,
        "/api/v1/telescopes/{id}",
        drogon::Get,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter"
      );
      ADD_METHOD_TO(
        TelescopeController::update,
        "/api/v1/telescopes/{id}",
        drogon::Put,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter"
      );
      ADD_METHOD_TO(
        TelescopeController::remove,
        "/api/v1/telescopes/{id}",
        drogon::Delete,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter"
      );

      METHOD_LIST_END

      TelescopeController();

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
