#pragma once

#include <cmath>
#include <cstdio>
#include <optional>
#include <string>

namespace Nyx::Core {
  class TimeConversion {
    public:
      static auto iso8601_to_jd(
        const std::string& iso_timestamp
      ) -> std::optional<double> {
        auto year = 0;
        auto month = 0;
        auto day = 0;
        auto hour = 0;
        auto minute = 0;
        auto second = 0.0;

        auto count = std::sscanf(
          iso_timestamp.c_str(),
          "%d-%d-%dT%d:%d:%lf",
          &year, &month, &day,
          &hour, &minute, &second
        );

        if (count < 6) {
          count = std::sscanf(
            iso_timestamp.c_str(),
            "%d-%d-%d %d:%d:%lf",
            &year, &month, &day,
            &hour, &minute, &second
          );
        }

        if (count < 3) return std::nullopt;

        auto a_year = year;
        auto a_month = month;
        if (a_month <= 2) {
          a_year -= 1;
          a_month += 12;
        }

        auto a = a_year / 100;
        auto b = 2 - a + a / 4;

        auto jd = std::floor(
          365.25 * (a_year + 4716)
        ) + std::floor(
          30.6001 * (a_month + 1)
        ) + day + b - 1524.5;

        jd += (hour + minute / 60.0
          + second / 3600.0) / 24.0;

        return jd;
      }

      static constexpr double btjd_offset = 2457000.0;
  };
} // namespace Nyx::Core
