#include "infrastructure/imaging/Exiv2ExifParser.hpp"

#include <cmath>
#include <exiv2/exiv2.hpp>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Imaging {
  static auto dms_to_decimal(
    const Exiv2::Value& value
  ) -> double {
    auto degrees = value.toFloat(0);
    auto minutes = value.toFloat(1);
    auto seconds = value.toFloat(2);
    return degrees + minutes / 60.0 + seconds / 3600.0;
  }

  auto Exiv2ExifParser::parse(
    const std::string& file_path
  ) -> Nyx::Core::Result<
    Nyx::Application::Observation::ExifData
  > {
    try {
      auto image = Exiv2::ImageFactory::open(file_path);
      image->readMetadata();

      auto& exif_data = image->exifData();
      auto result = Nyx::Application::Observation::ExifData{};

      if (exif_data.empty()) {
        spdlog::debug(
          "No EXIF data found in file: {}", file_path
        );
        return result;
      }

      auto find_key = [&](const std::string& key)
        -> Exiv2::ExifData::const_iterator {
        return exif_data.findKey(Exiv2::ExifKey(key));
      };

      auto date_time = find_key("Exif.Photo.DateTimeOriginal");
      if (date_time != exif_data.end()) {
        result.captured_at = date_time->toString();
      }

      auto camera = find_key("Exif.Image.Model");
      if (camera != exif_data.end()) {
        result.camera_model = camera->toString();
      }

      auto exposure = find_key("Exif.Photo.ExposureTime");
      if (exposure != exif_data.end()) {
        result.exposure_time_seconds =
          exposure->toFloat();
      }

      auto iso = find_key("Exif.Photo.ISOSpeedRatings");
      if (iso != exif_data.end()) {
        result.iso_speed =
          static_cast<int>(iso->toInt64());
      }

      auto gps_lat = find_key("Exif.GPSInfo.GPSLatitude");
      auto gps_lat_ref = find_key("Exif.GPSInfo.GPSLatitudeRef");
      if (gps_lat != exif_data.end()
          && gps_lat_ref != exif_data.end()) {
        auto latitude = dms_to_decimal(gps_lat->value());
        if (gps_lat_ref->toString() == "S") {
          latitude = -latitude;
        }
        result.gps_latitude = latitude;
      }

      auto gps_lon = find_key("Exif.GPSInfo.GPSLongitude");
      auto gps_lon_ref = find_key("Exif.GPSInfo.GPSLongitudeRef");
      if (gps_lon != exif_data.end()
          && gps_lon_ref != exif_data.end()) {
        auto longitude = dms_to_decimal(gps_lon->value());
        if (gps_lon_ref->toString() == "W") {
          longitude = -longitude;
        }
        result.gps_longitude = longitude;
      }

      auto width = find_key("Exif.Photo.PixelXDimension");
      if (width != exif_data.end()) {
        result.image_width =
          static_cast<int>(width->toInt64());
      }

      auto height = find_key("Exif.Photo.PixelYDimension");
      if (height != exif_data.end()) {
        result.image_height =
          static_cast<int>(height->toInt64());
      }

      spdlog::debug(
        "Parsed EXIF data from file: {}", file_path
      );
      return result;
    } catch (const Exiv2::Error& e) {
      spdlog::warn(
        "Failed to parse EXIF data from {}: {}",
        file_path, e.what()
      );
      return Nyx::Application::Observation::ExifData{};
    }
  }
} // namespace Nyx::Infrastructure::Imaging
