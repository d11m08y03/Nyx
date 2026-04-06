#pragma once

#include "application/observation/ObservationService.hpp"

#include <drogon/HttpController.h>
#include <memory>

namespace Nyx::Presentation::Http::Observation {
  class ObservationImageController
    : public drogon::HttpController<
        ObservationImageController
      > {
    public:
      METHOD_LIST_BEGIN

      ADD_METHOD_TO(
        ObservationImageController::upload,
        "/api/v1/observation-sessions/{session_id}/images",
        drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter",
        "Nyx::Presentation::Middleware::CsrfFilter"
      );
      ADD_METHOD_TO(
        ObservationImageController::remove,
        "/api/v1/observation-sessions/{session_id}/images/{image_id}",
        drogon::Delete,
        "Nyx::Presentation::Middleware::CorrelationIdFilter",
        "Nyx::Presentation::Middleware::JwtAuthFilter",
        "Nyx::Presentation::Middleware::CsrfFilter"
      );

      METHOD_LIST_END

      ObservationImageController();

      auto upload(
        const drogon::HttpRequestPtr& request,
        std::function<void(
          const drogon::HttpResponsePtr&
        )>&& callback,
        const std::string& session_id
      ) -> void;

      auto remove(
        const drogon::HttpRequestPtr& request,
        std::function<void(
          const drogon::HttpResponsePtr&
        )>&& callback,
        const std::string& session_id,
        const std::string& image_id
      ) -> void;

    private:
      std::shared_ptr<
        Nyx::Application::Observation::ObservationService
      > observation_service;
  };
} // namespace Nyx::Presentation::Http::Observation
