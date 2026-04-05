#pragma once

#include "application/target/IMastClient.hpp"

#include <string>

namespace Nyx::Infrastructure::Nasa {
  class MastClient : public Nyx::Application::Target::IMastClient {
    public:
      explicit MastClient(std::string base_url);

      auto resolve_target(
        const std::string& name
      ) -> Nyx::Core::Result<
        Nyx::Application::Target::ResolvedTarget
      > override;

      auto search_tess_timeseries(
        double ra, double dec, double radius
      ) -> Nyx::Core::Result<
        std::vector<Nyx::Application::Target::MastObservation>
      > override;

      auto get_data_products(
        const std::string& obsid
      ) -> Nyx::Core::Result<
        std::vector<Nyx::Application::Target::DataProduct>
      > override;

      auto download_fits(
        const std::string& data_uri
      ) -> Nyx::Core::Result<std::string> override;

    private:
      auto invoke_mast_service(
        const std::string& request_json
      ) -> Nyx::Core::Result<std::string>;

      std::string base_url;
  };
} // namespace Nyx::Infrastructure::Nasa
