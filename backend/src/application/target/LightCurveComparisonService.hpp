#pragma once

#include "application/target/Dtos.hpp"
#include "core/error/AppError.hpp"
#include "domain/repositories/ILightCurvePointRepository.hpp"
#include "domain/repositories/IObservationImageRepository.hpp"
#include "domain/repositories/IObservationSessionRepository.hpp"
#include "domain/repositories/ITargetRepository.hpp"
#include "domain/repositories/ITessObservationRepository.hpp"

#include <memory>
#include <spdlog/spdlog.h>

namespace Nyx::Application::Target {
  class LightCurveComparisonService {
    public:
      LightCurveComparisonService(
        std::shared_ptr<
          Nyx::Domain::ITargetRepository
        > target_repository,
        std::shared_ptr<
          Nyx::Domain::ITessObservationRepository
        > tess_observation_repository,
        std::shared_ptr<
          Nyx::Domain::ILightCurvePointRepository
        > light_curve_point_repository,
        std::shared_ptr<
          Nyx::Domain::IObservationSessionRepository
        > session_repository,
        std::shared_ptr<
          Nyx::Domain::IObservationImageRepository
        > image_repository
      );

      auto get_comparison(
        const std::string& user_id,
        const std::string& target_id,
        const std::string& tess_observation_id,
        bool quality_filter,
        std::shared_ptr<spdlog::logger> logger
      ) -> Nyx::Core::Result<
        LightCurveComparisonResponse
      >;

    private:
      std::shared_ptr<Nyx::Domain::ITargetRepository>
        target_repository;
      std::shared_ptr<
        Nyx::Domain::ITessObservationRepository
      > tess_observation_repository;
      std::shared_ptr<
        Nyx::Domain::ILightCurvePointRepository
      > light_curve_point_repository;
      std::shared_ptr<
        Nyx::Domain::IObservationSessionRepository
      > session_repository;
      std::shared_ptr<
        Nyx::Domain::IObservationImageRepository
      > image_repository;
  };
} // namespace Nyx::Application::Target
