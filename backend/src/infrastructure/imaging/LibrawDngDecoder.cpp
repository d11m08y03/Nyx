#include "infrastructure/imaging/LibrawDngDecoder.hpp"

#include <libraw/libraw.h>
#include <spdlog/spdlog.h>

namespace Nyx::Infrastructure::Imaging {
  auto LibrawDngDecoder::decode(
    const std::string& file_path
  ) -> Nyx::Core::Result<
    Nyx::Application::Observation::DecodedImage
  > {
    auto processor = LibRaw{};

    auto open_result = processor.open_file(
      file_path.c_str()
    );
    if (open_result != LIBRAW_SUCCESS) {
      spdlog::error(
        "LibRaw failed to open file {}: {}",
        file_path, libraw_strerror(open_result)
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to decode DNG file"
        )
      );
    }

    auto unpack_result = processor.unpack();
    if (unpack_result != LIBRAW_SUCCESS) {
      spdlog::error(
        "LibRaw failed to unpack {}: {}",
        file_path, libraw_strerror(unpack_result)
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to unpack DNG file"
        )
      );
    }

    processor.imgdata.params.gamm[0] = 1.0;
    processor.imgdata.params.gamm[1] = 1.0;
    processor.imgdata.params.output_bps = 16;
    processor.imgdata.params.no_auto_bright = 1;
    processor.imgdata.params.use_camera_wb = 1;

    auto process_result = processor.dcraw_process();
    if (process_result != LIBRAW_SUCCESS) {
      spdlog::error(
        "LibRaw failed to process {}: {}",
        file_path, libraw_strerror(process_result)
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to process DNG file"
        )
      );
    }

    auto* image = processor.dcraw_make_mem_image();
    if (image == nullptr) {
      spdlog::error(
        "LibRaw failed to create memory image for {}",
        file_path
      );
      return std::unexpected(
        Nyx::Core::AppError::internal(
          "Failed to create image from DNG"
        )
      );
    }

    auto width = static_cast<int>(image->width);
    auto height = static_cast<int>(image->height);
    auto channels = static_cast<int>(image->colors);
    auto pixel_count = static_cast<std::size_t>(
      width * height * channels
    );

    auto pixels = std::vector<uint16_t>(pixel_count);
    auto* raw_data =
      reinterpret_cast<uint16_t*>(image->data);
    std::copy(
      raw_data, raw_data + pixel_count, pixels.begin()
    );

    processor.dcraw_clear_mem(image);
    processor.recycle();

    spdlog::debug(
      "Decoded DNG {}: {}x{}, {} channels",
      file_path, width, height, channels
    );

    return Nyx::Application::Observation::DecodedImage{
      .pixels = std::move(pixels),
      .width = width,
      .height = height,
      .channels = channels,
    };
  }
} // namespace Nyx::Infrastructure::Imaging
