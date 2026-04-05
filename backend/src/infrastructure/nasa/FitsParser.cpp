#include "infrastructure/nasa/FitsParser.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fitsio.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Nasa {
  class FitsFileGuard {
    public:
      explicit FitsFileGuard(const std::string& path)
        : path(path) {}

      ~FitsFileGuard() {
        if (!this->path.empty()) {
          std::remove(this->path.c_str());
        }
      }

      FitsFileGuard(const FitsFileGuard&) = delete;
      auto operator=(const FitsFileGuard&)
        -> FitsFileGuard& = delete;

    private:
      std::string path;
  };

  auto FitsParser::parse_light_curve(
    const std::string& fits_data
  ) -> Nyx::Core::Result<
    std::vector<Nyx::Domain::LightCurvePoint>
  > {
    if (fits_data.empty()) {
      return std::unexpected(
        Nyx::Core::AppError::validation("Empty FITS data")
      );
    }

    auto temp_template =
      std::string{"/tmp/nyx_fits_XXXXXX"};
    auto fd = mkstemp(temp_template.data());
    if (fd < 0) {
      spdlog::error("Failed to create temp file for FITS data");
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to create temp file"
        )
      );
    }

    auto guard = FitsFileGuard{temp_template};

    auto bytes_written = write(
      fd, fits_data.data(), fits_data.size()
    );
    close(fd);

    if (bytes_written
      != static_cast<ssize_t>(fits_data.size())) {
      spdlog::error("Failed to write FITS data to temp file");
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to write FITS data"
        )
      );
    }

    spdlog::debug(
      "Wrote {} bytes to temp FITS file: {}",
      fits_data.size(), temp_template
    );

    fitsfile* fptr = nullptr;
    auto status = 0;

    fits_open_file(&fptr, temp_template.c_str(), READONLY, &status);
    if (status != 0) {
      char error_text[FLEN_STATUS];
      fits_get_errstatus(status, error_text);
      spdlog::error(
        "Failed to open FITS file: {}", error_text
      );
      return std::unexpected(Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "Failed to open FITS file", {}
      });
    }

    fits_movabs_hdu(fptr, 2, nullptr, &status);
    if (status != 0) {
      char error_text[FLEN_STATUS];
      fits_get_errstatus(status, error_text);
      spdlog::error(
        "Failed to move to HDU 2: {}", error_text
      );
      fits_close_file(fptr, &status);
      return std::unexpected(Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "FITS file missing binary table extension", {}
      });
    }

    long num_rows = 0;
    fits_get_num_rows(fptr, &num_rows, &status);
    if (status != 0 || num_rows <= 0) {
      spdlog::warn("FITS binary table has no rows");
      fits_close_file(fptr, &status);
      return std::vector<Nyx::Domain::LightCurvePoint>{};
    }

    spdlog::debug("FITS binary table has {} rows", num_rows);

    auto time_col = 0;
    auto pdcsap_col = 0;
    auto pdcsap_err_col = 0;
    auto sap_col = 0;
    auto quality_col = 0;

    fits_get_colnum(
      fptr, CASEINSEN, const_cast<char*>("TIME"),
      &time_col, &status
    );
    if (status != 0) {
      spdlog::error("FITS file missing TIME column");
      fits_close_file(fptr, &status);
      return std::unexpected(Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "FITS file missing TIME column", {}
      });
    }

    status = 0;
    fits_get_colnum(
      fptr, CASEINSEN, const_cast<char*>("PDCSAP_FLUX"),
      &pdcsap_col, &status
    );
    if (status != 0) {
      spdlog::warn("FITS file missing PDCSAP_FLUX column");
      pdcsap_col = 0;
      status = 0;
    }

    fits_get_colnum(
      fptr, CASEINSEN, const_cast<char*>("PDCSAP_FLUX_ERR"),
      &pdcsap_err_col, &status
    );
    if (status != 0) {
      spdlog::warn(
        "FITS file missing PDCSAP_FLUX_ERR column"
      );
      pdcsap_err_col = 0;
      status = 0;
    }

    fits_get_colnum(
      fptr, CASEINSEN, const_cast<char*>("SAP_FLUX"),
      &sap_col, &status
    );
    if (status != 0) {
      spdlog::warn("FITS file missing SAP_FLUX column");
      sap_col = 0;
      status = 0;
    }

    fits_get_colnum(
      fptr, CASEINSEN, const_cast<char*>("QUALITY"),
      &quality_col, &status
    );
    if (status != 0) {
      spdlog::warn("FITS file missing QUALITY column");
      quality_col = 0;
      status = 0;
    }

    auto time_values = std::vector<double>(num_rows);
    auto pdcsap_values = std::vector<float>(num_rows);
    auto pdcsap_err_values = std::vector<float>(num_rows);
    auto sap_values = std::vector<float>(num_rows);
    auto quality_values = std::vector<int>(num_rows);

    auto time_nulls = std::vector<int>(num_rows, 0);
    auto pdcsap_nulls = std::vector<int>(num_rows, 0);
    auto pdcsap_err_nulls = std::vector<int>(num_rows, 0);
    auto sap_nulls = std::vector<int>(num_rows, 0);

    auto any_null = 0;

    fits_read_col(
      fptr, TDOUBLE, time_col, 1, 1, num_rows,
      nullptr, time_values.data(), &any_null, &status
    );

    if (pdcsap_col > 0) {
      auto nan_val = std::numeric_limits<float>::quiet_NaN();
      fits_read_col(
        fptr, TFLOAT, pdcsap_col, 1, 1, num_rows,
        &nan_val, pdcsap_values.data(), &any_null, &status
      );
    }

    if (pdcsap_err_col > 0) {
      auto nan_val = std::numeric_limits<float>::quiet_NaN();
      fits_read_col(
        fptr, TFLOAT, pdcsap_err_col, 1, 1, num_rows,
        &nan_val, pdcsap_err_values.data(), &any_null, &status
      );
    }

    if (sap_col > 0) {
      auto nan_val = std::numeric_limits<float>::quiet_NaN();
      fits_read_col(
        fptr, TFLOAT, sap_col, 1, 1, num_rows,
        &nan_val, sap_values.data(), &any_null, &status
      );
    }

    if (quality_col > 0) {
      fits_read_col(
        fptr, TINT, quality_col, 1, 1, num_rows,
        nullptr, quality_values.data(), &any_null, &status
      );
    }

    fits_close_file(fptr, &status);

    if (status != 0) {
      char error_text[FLEN_STATUS];
      fits_get_errstatus(status, error_text);
      spdlog::error(
        "Error reading FITS columns: {}", error_text
      );
      return std::unexpected(Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "Error reading FITS columns", {}
      });
    }

    auto points = std::vector<Nyx::Domain::LightCurvePoint>{};
    points.reserve(num_rows);

    for (auto i = long{0}; i < num_rows; ++i) {
      if (std::isnan(time_values[i])) continue;

      auto point = Nyx::Domain::LightCurvePoint{
        .id = 0,
        .tess_observation_id = "",
        .time = time_values[i],
        .pdcsap_flux = (pdcsap_col > 0
          && !std::isnan(pdcsap_values[i]))
          ? std::optional<float>(pdcsap_values[i])
          : std::nullopt,
        .pdcsap_flux_err = (pdcsap_err_col > 0
          && !std::isnan(pdcsap_err_values[i]))
          ? std::optional<float>(pdcsap_err_values[i])
          : std::nullopt,
        .sap_flux = (sap_col > 0
          && !std::isnan(sap_values[i]))
          ? std::optional<float>(sap_values[i])
          : std::nullopt,
        .quality = (quality_col > 0)
          ? quality_values[i] : 0,
      };

      points.push_back(point);
    }

    spdlog::info(
      "Parsed {} light curve points from FITS ({} rows total)",
      points.size(), num_rows
    );

    return points;
  }
} // namespace Nyx::Infrastructure::Nasa
