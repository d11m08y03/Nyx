#pragma once

#include "application/target/TargetService.hpp"

#include <drogon/HttpController.h>
#include <memory>

namespace Nyx::Presentation::Http::Target {
  class TargetController
    : public drogon::HttpController<TargetController> {
    public:
      METHOD_LIST_BEGIN

      ADD_METHOD_TO(
        TargetController::resolve,
        "/api/v1/targets/resolve",
        drogon::Post,
        "Nyx::Presentation::Middleware::CorrelationIdFilter"
      );
      ADD_METHOD_TO(
        TargetController::get_by_id,
        "/api/v1/targets/{1}",
        drogon::Get,
        "Nyx::Presentation::Middleware::CorrelationIdFilter"
      );
      ADD_METHOD_TO(
        TargetController::list_tess_observations,
        "/api/v1/targets/{1}/tess-observations",
        drogon::Get,
        "Nyx::Presentation::Middleware::CorrelationIdFilter"
      );

      METHOD_LIST_END

      TargetController();

      auto resolve(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
      ) -> void;

      auto get_by_id(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        const std::string& id
      ) -> void;

      auto list_tess_observations(
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
