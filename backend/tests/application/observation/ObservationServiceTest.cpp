#include "application/observation/ObservationService.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace Nyx::Application::Observation::Tests {
  class MockSessionRepository
    : public Nyx::Domain::IObservationSessionRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::ObservationSession>,
        create,
        (const Nyx::Domain::ObservationSession& session),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::ObservationSession>>),
        find_by_user_id,
        (const std::string& user_id),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::ObservationSession>>),
        find_by_id,
        (const std::string& id),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::ObservationSession>>),
        find_by_user_id_and_target_id,
        (const std::string& user_id,
         const std::string& target_id),
        (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::ObservationSession>,
        update,
        (const Nyx::Domain::ObservationSession& session),
        (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, remove,
        (const std::string& id), (override)
      );
  };

  class MockImageRepository
    : public Nyx::Domain::IObservationImageRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::ObservationImage>,
        create,
        (const Nyx::Domain::ObservationImage& image),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::ObservationImage>>),
        find_by_session_id,
        (const std::string& session_id),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::ObservationImage>>),
        find_by_id,
        (const std::string& id),
        (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, remove,
        (const std::string& id), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::ObservationImage>,
        update_photometry,
        (const Nyx::Domain::ObservationImage& image),
        (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>,
        update_photometry_status_batch,
        (const std::string& session_id,
         const std::string& status),
        (override)
      );
  };

  class MockTargetRepository
    : public Nyx::Domain::ITargetRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::Target>, create,
        (const Nyx::Domain::Target& target), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::Target>>),
        find_by_id,
        (const std::string& id),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::Target>>),
        find_by_canonical_name,
        (const std::string& canonical_name),
        (override)
      );
  };

  class MockTelescopeRepository
    : public Nyx::Domain::ITelescopeRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::Telescope>, create,
        (const Nyx::Domain::Telescope& telescope), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::Telescope>>),
        find_by_user_id,
        (const std::string& user_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::Telescope>>),
        find_by_id,
        (const std::string& id), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::Telescope>, update,
        (const Nyx::Domain::Telescope& telescope), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, remove,
        (const std::string& id), (override)
      );
  };

  class MockCameraRepository
    : public Nyx::Domain::ICameraRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::Camera>, create,
        (const Nyx::Domain::Camera& camera), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::Camera>>),
        find_by_user_id,
        (const std::string& user_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::Camera>>),
        find_by_id,
        (const std::string& id), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::Camera>, update,
        (const Nyx::Domain::Camera& camera), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, remove,
        (const std::string& id), (override)
      );
  };

  class MockMountRepository
    : public Nyx::Domain::IMountRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::Mount>, create,
        (const Nyx::Domain::Mount& mount), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::Mount>>),
        find_by_user_id,
        (const std::string& user_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::Mount>>),
        find_by_id,
        (const std::string& id), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::Mount>, update,
        (const Nyx::Domain::Mount& mount), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, remove,
        (const std::string& id), (override)
      );
  };

  class MockLocationRepository
    : public Nyx::Domain::IObservingLocationRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::ObservingLocation>,
        create,
        (const Nyx::Domain::ObservingLocation& location),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::ObservingLocation>>),
        find_by_user_id,
        (const std::string& user_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::ObservingLocation>>),
        find_by_id,
        (const std::string& id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::ObservingLocation>>),
        find_by_user_id_and_name,
        (const std::string& user_id, const std::string& name),
        (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::ObservingLocation>,
        update,
        (const Nyx::Domain::ObservingLocation& location),
        (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, remove,
        (const std::string& id), (override)
      );
  };

  class MockFilterRepository
    : public Nyx::Domain::IFilterRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::Filter>, create,
        (const Nyx::Domain::Filter& filter), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::Filter>>),
        find_by_user_id,
        (const std::string& user_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::Filter>>),
        find_by_id,
        (const std::string& id), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::Filter>, update,
        (const Nyx::Domain::Filter& filter), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, remove,
        (const std::string& id), (override)
      );
  };

  class MockExifParser : public IExifParser {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<ExifData>, parse,
        (const std::string& file_path), (override)
      );
  };

  class MockFileStorage : public IFileStorage {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<std::string>, save_file,
        (const std::string& directory,
         const std::string& filename,
         const char* data, std::size_t length),
        (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, remove_file,
        (const std::string& file_path), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, remove_directory,
        (const std::string& directory_path), (override)
      );
  };

  class MockDngDecoder : public IDngDecoder {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<DecodedImage>, decode,
        (const std::string& file_path), (override)
      );
  };

  class MockPhotometryProcessor
    : public IPhotometryProcessor {
    public:
      MOCK_METHOD(
        std::vector<DetectedSource>, detect_sources,
        (const DecodedImage& image,
         double threshold_sigma),
        (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<PhotometryResult>,
        measure_aperture,
        (const DecodedImage& image, int center_x,
         int center_y, double aperture_radius,
         double annulus_inner_radius,
         double annulus_outer_radius),
        (override)
      );
  };

  class MockUuidGenerator
    : public Nyx::Core::IUuidGenerator {
    public:
      MOCK_METHOD(std::string, generate, (), (override));
  };

  // --- Test data helpers ---

  static auto make_target() -> Nyx::Domain::Target {
    return {
      .id = "target-1",
      .canonical_name = "pi Men",
      .target_type = "star",
      .right_ascension = 84.29,
      .declination = -80.47,
    };
  }

  static auto make_telescope() -> Nyx::Domain::Telescope {
    return {
      .id = "tel-1", .user_id = "u-1",
      .name = "ED80", .aperture_mm = 80,
      .focal_length_mm = 600,
      .optical_design = "refractor",
      .brand = "Skywatcher", .model = "ED80",
    };
  }

  static auto make_camera() -> Nyx::Domain::Camera {
    return {
      .id = "cam-1", .user_id = "u-1",
      .name = "ASI294MC", .sensor_type = "CMOS",
      .brand = "ZWO", .model = "ASI294MC Pro",
      .pixel_size_um = 4.63,
      .resolution = "4144x2822",
    };
  }

  static auto make_mount() -> Nyx::Domain::Mount {
    return {
      .id = "mnt-1", .user_id = "u-1",
      .name = "EQ6-R Pro", .mount_type = "equatorial",
      .is_tracked = true, .has_goto = true,
      .brand = "Skywatcher", .model = "EQ6-R Pro",
    };
  }

  static auto make_location()
    -> Nyx::Domain::ObservingLocation {
    return {
      .id = "loc-1", .user_id = "u-1",
      .name = "Backyard", .latitude = -20.16,
      .longitude = 57.50,
      .bortle_class = 5,
    };
  }

  static auto make_filter() -> Nyx::Domain::Filter {
    return {
      .id = "flt-1", .user_id = "u-1",
      .name = "L-Pro", .band = "broadband",
      .bandwidth_nm = 300.0,
      .brand = "Optolong", .model = "L-Pro",
    };
  }

  static auto make_session()
    -> Nyx::Domain::ObservationSession {
    return {
      .id = "ses-1", .user_id = "u-1",
      .target_id = "target-1",
      .telescope_id = "tel-1",
      .camera_id = "cam-1", .mount_id = "mnt-1",
      .location_id = "loc-1",
      .filter_id = std::nullopt,
      .notes = "First light",
      .created_at = "2026-04-05T00:00:00Z",
      .updated_at = "2026-04-05T00:00:00Z",
    };
  }

  static auto make_image()
    -> Nyx::Domain::ObservationImage {
    return {
      .id = "img-1", .session_id = "ses-1",
      .filename = "img-1.dng",
      .original_filename = "star.dng",
      .file_path = "u-1/ses-1/img-1.dng",
      .file_size_bytes = 1024,
      .mime_type = "image/x-adobe-dng",
      .captured_at = "2026-04-05T00:00:00Z",
      .camera_model = "Pixel 7",
      .exposure_time_seconds = 30.0,
      .iso_speed = 3200,
      .gps_latitude = -20.16,
      .gps_longitude = 57.50,
      .image_width = 4032,
      .image_height = 3024,
      .target_x = std::nullopt,
      .target_y = std::nullopt,
      .raw_flux = std::nullopt,
      .raw_flux_error = std::nullopt,
      .relative_flux = std::nullopt,
      .relative_flux_error = std::nullopt,
      .photometry_status = std::nullopt,
      .photometry_error_message = std::nullopt,
      .created_at = "2026-04-05T00:00:00Z",
    };
  }

  static auto make_create_request()
    -> CreateSessionRequest {
    return {
      .target_id = "target-1",
      .telescope_id = "tel-1",
      .camera_id = "cam-1",
      .mount_id = "mnt-1",
      .location_id = "loc-1",
      .filter_id = std::nullopt,
      .notes = "First light",
    };
  }

  // --- Fixture ---

  class ObservationServiceTest
    : public ::testing::Test {
    protected:
      auto SetUp() -> void override {
        this->session_repo =
          std::make_shared<MockSessionRepository>();
        this->image_repo =
          std::make_shared<MockImageRepository>();
        this->target_repo =
          std::make_shared<MockTargetRepository>();
        this->telescope_repo =
          std::make_shared<MockTelescopeRepository>();
        this->camera_repo =
          std::make_shared<MockCameraRepository>();
        this->mount_repo =
          std::make_shared<MockMountRepository>();
        this->location_repo =
          std::make_shared<MockLocationRepository>();
        this->filter_repo =
          std::make_shared<MockFilterRepository>();
        this->exif_parser =
          std::make_shared<MockExifParser>();
        this->file_storage =
          std::make_shared<MockFileStorage>();
        this->dng_decoder =
          std::make_shared<MockDngDecoder>();
        this->photometry =
          std::make_shared<MockPhotometryProcessor>();
        this->uuid_gen =
          std::make_shared<MockUuidGenerator>();
        this->logger = spdlog::default_logger();

        this->service =
          std::make_unique<ObservationService>(
            this->session_repo, this->image_repo,
            this->target_repo, this->telescope_repo,
            this->camera_repo, this->mount_repo,
            this->location_repo, this->filter_repo,
            this->exif_parser, this->file_storage,
            this->dng_decoder, this->photometry,
            this->uuid_gen
          );
      }

      auto expect_equipment_found() -> void {
        EXPECT_CALL(*this->telescope_repo, find_by_id("tel-1"))
          .WillOnce(::testing::Return(
            std::optional{make_telescope()}
          ));
        EXPECT_CALL(*this->camera_repo, find_by_id("cam-1"))
          .WillOnce(::testing::Return(
            std::optional{make_camera()}
          ));
        EXPECT_CALL(*this->mount_repo, find_by_id("mnt-1"))
          .WillOnce(::testing::Return(
            std::optional{make_mount()}
          ));
        EXPECT_CALL(*this->location_repo, find_by_id("loc-1"))
          .WillOnce(::testing::Return(
            std::optional{make_location()}
          ));
      }

      auto expect_session_found(
        const std::string& session_id = "ses-1"
      ) -> void {
        EXPECT_CALL(
          *this->session_repo, find_by_id(session_id)
        ).WillOnce(::testing::Return(
          std::optional{make_session()}
        ));
      }

      std::shared_ptr<MockSessionRepository> session_repo;
      std::shared_ptr<MockImageRepository> image_repo;
      std::shared_ptr<MockTargetRepository> target_repo;
      std::shared_ptr<MockTelescopeRepository>
        telescope_repo;
      std::shared_ptr<MockCameraRepository> camera_repo;
      std::shared_ptr<MockMountRepository> mount_repo;
      std::shared_ptr<MockLocationRepository> location_repo;
      std::shared_ptr<MockFilterRepository> filter_repo;
      std::shared_ptr<MockExifParser> exif_parser;
      std::shared_ptr<MockFileStorage> file_storage;
      std::shared_ptr<MockDngDecoder> dng_decoder;
      std::shared_ptr<MockPhotometryProcessor> photometry;
      std::shared_ptr<MockUuidGenerator> uuid_gen;
      std::shared_ptr<spdlog::logger> logger;
      std::unique_ptr<ObservationService> service;
  };

  // --- Create Session ---

  TEST_F(ObservationServiceTest, CreateSessionSuccess) {
    auto request = make_create_request();
    auto target = make_target();
    auto session = make_session();

    EXPECT_CALL(*this->target_repo, find_by_id("target-1"))
      .WillOnce(::testing::Return(std::optional{target}));
    this->expect_equipment_found();
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("ses-1"));
    EXPECT_CALL(*this->session_repo, create(::testing::_))
      .WillOnce(::testing::Return(session));

    auto result = this->service->create_session(
      "u-1", request, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "ses-1");
    EXPECT_EQ(result->target_id, "target-1");
    EXPECT_EQ(result->telescope_id, "tel-1");
    EXPECT_EQ(result->camera_id, "cam-1");
    EXPECT_EQ(result->mount_id, "mnt-1");
    EXPECT_EQ(result->location_id, "loc-1");
    EXPECT_TRUE(result->images.empty());
  }

  TEST_F(
    ObservationServiceTest, CreateSessionWithFilter
  ) {
    auto request = make_create_request();
    request.filter_id = "flt-1";
    auto target = make_target();
    auto session = make_session();
    session.filter_id = "flt-1";

    EXPECT_CALL(*this->target_repo, find_by_id("target-1"))
      .WillOnce(::testing::Return(std::optional{target}));
    this->expect_equipment_found();
    EXPECT_CALL(*this->filter_repo, find_by_id("flt-1"))
      .WillOnce(::testing::Return(
        std::optional{make_filter()}
      ));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("ses-1"));
    EXPECT_CALL(*this->session_repo, create(::testing::_))
      .WillOnce(::testing::Return(session));

    auto result = this->service->create_session(
      "u-1", request, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->filter_id, "flt-1");
  }

  TEST_F(
    ObservationServiceTest, CreateSessionTargetNotFound
  ) {
    EXPECT_CALL(*this->target_repo, find_by_id("target-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>{}
      ));

    auto result = this->service->create_session(
      "u-1", make_create_request(), this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::ResourceNotFound
    );
  }

  TEST_F(
    ObservationServiceTest,
    CreateSessionTelescopeNotOwned
  ) {
    auto target = make_target();
    auto other_telescope = make_telescope();
    other_telescope.user_id = "other-user";

    EXPECT_CALL(*this->target_repo, find_by_id("target-1"))
      .WillOnce(::testing::Return(std::optional{target}));
    EXPECT_CALL(*this->telescope_repo, find_by_id("tel-1"))
      .WillOnce(::testing::Return(
        std::optional{other_telescope}
      ));

    auto result = this->service->create_session(
      "u-1", make_create_request(), this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::PermissionDenied
    );
  }

  TEST_F(
    ObservationServiceTest,
    CreateSessionCameraNotFound
  ) {
    auto target = make_target();

    EXPECT_CALL(*this->target_repo, find_by_id("target-1"))
      .WillOnce(::testing::Return(std::optional{target}));
    EXPECT_CALL(*this->telescope_repo, find_by_id("tel-1"))
      .WillOnce(::testing::Return(
        std::optional{make_telescope()}
      ));
    EXPECT_CALL(*this->camera_repo, find_by_id("cam-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Camera>{}
      ));

    auto result = this->service->create_session(
      "u-1", make_create_request(), this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::ResourceNotFound
    );
  }

  TEST_F(
    ObservationServiceTest,
    CreateSessionFilterNotOwned
  ) {
    auto request = make_create_request();
    request.filter_id = "flt-1";
    auto target = make_target();
    auto other_filter = make_filter();
    other_filter.user_id = "other-user";

    EXPECT_CALL(*this->target_repo, find_by_id("target-1"))
      .WillOnce(::testing::Return(std::optional{target}));
    this->expect_equipment_found();
    EXPECT_CALL(*this->filter_repo, find_by_id("flt-1"))
      .WillOnce(::testing::Return(
        std::optional{other_filter}
      ));

    auto result = this->service->create_session(
      "u-1", request, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::PermissionDenied
    );
  }

  TEST_F(
    ObservationServiceTest, CreateSessionDatabaseError
  ) {
    auto target = make_target();

    EXPECT_CALL(*this->target_repo, find_by_id("target-1"))
      .WillOnce(::testing::Return(std::optional{target}));
    this->expect_equipment_found();
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("ses-1"));
    EXPECT_CALL(*this->session_repo, create(::testing::_))
      .WillOnce(::testing::Return(std::unexpected(
        Nyx::Core::AppError::internal("DB error")
      )));

    auto result = this->service->create_session(
      "u-1", make_create_request(), this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::InternalError
    );
  }

  // --- List Sessions ---

  TEST_F(ObservationServiceTest, ListSessionsSuccess) {
    auto sessions =
      std::vector<Nyx::Domain::ObservationSession>{
        make_session()
      };

    EXPECT_CALL(
      *this->session_repo, find_by_user_id("u-1")
    ).WillOnce(::testing::Return(sessions));
    EXPECT_CALL(
      *this->image_repo, find_by_session_id("ses-1")
    ).WillOnce(::testing::Return(
      std::vector<Nyx::Domain::ObservationImage>{}
    ));

    auto result = this->service->list_sessions(
      "u-1", this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 1);
    EXPECT_EQ(result->at(0).id, "ses-1");
  }

  TEST_F(
    ObservationServiceTest, ListSessionsEmpty
  ) {
    EXPECT_CALL(
      *this->session_repo, find_by_user_id("u-1")
    ).WillOnce(::testing::Return(
      std::vector<Nyx::Domain::ObservationSession>{}
    ));

    auto result = this->service->list_sessions(
      "u-1", this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
  }

  // --- Get Session ---

  TEST_F(ObservationServiceTest, GetSessionSuccess) {
    this->expect_session_found();
    EXPECT_CALL(
      *this->image_repo, find_by_session_id("ses-1")
    ).WillOnce(::testing::Return(
      std::vector{make_image()}
    ));

    auto result = this->service->get_session(
      "u-1", "ses-1", this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "ses-1");
    EXPECT_EQ(result->images.size(), 1);
    EXPECT_EQ(result->images[0].id, "img-1");
  }

  TEST_F(ObservationServiceTest, GetSessionNotFound) {
    EXPECT_CALL(
      *this->session_repo, find_by_id("ses-99")
    ).WillOnce(::testing::Return(
      std::optional<Nyx::Domain::ObservationSession>{}
    ));

    auto result = this->service->get_session(
      "u-1", "ses-99", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::ResourceNotFound
    );
  }

  TEST_F(
    ObservationServiceTest, GetSessionOwnershipDenied
  ) {
    auto session = make_session();
    session.user_id = "other-user";

    EXPECT_CALL(
      *this->session_repo, find_by_id("ses-1")
    ).WillOnce(::testing::Return(std::optional{session}));

    auto result = this->service->get_session(
      "u-1", "ses-1", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::PermissionDenied
    );
  }

  // --- Update Session ---

  TEST_F(ObservationServiceTest, UpdateSessionSuccess) {
    auto updated_session = make_session();
    updated_session.notes = "Updated notes";

    this->expect_session_found();
    EXPECT_CALL(
      *this->session_repo, update(::testing::_)
    ).WillOnce(::testing::Return(updated_session));
    EXPECT_CALL(
      *this->image_repo, find_by_session_id("ses-1")
    ).WillOnce(::testing::Return(
      std::vector<Nyx::Domain::ObservationImage>{}
    ));

    auto request = UpdateSessionRequest{
      .filter_id = std::nullopt,
      .notes = "Updated notes",
    };

    auto result = this->service->update_session(
      "u-1", "ses-1", request, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->notes, "Updated notes");
  }

  TEST_F(
    ObservationServiceTest, UpdateSessionWithFilter
  ) {
    auto updated_session = make_session();
    updated_session.filter_id = "flt-1";

    this->expect_session_found();
    EXPECT_CALL(
      *this->session_repo, update(::testing::_)
    ).WillOnce(::testing::Return(updated_session));
    EXPECT_CALL(
      *this->image_repo, find_by_session_id("ses-1")
    ).WillOnce(::testing::Return(
      std::vector<Nyx::Domain::ObservationImage>{}
    ));

    auto request = UpdateSessionRequest{
      .filter_id = "flt-1",
      .notes = std::nullopt,
    };

    auto result = this->service->update_session(
      "u-1", "ses-1", request, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->filter_id, "flt-1");
  }

  // --- Delete Session ---

  TEST_F(ObservationServiceTest, DeleteSessionSuccess) {
    this->expect_session_found();
    EXPECT_CALL(*this->session_repo, remove("ses-1"))
      .WillOnce(::testing::Return(Nyx::Core::Result<void>{}));
    EXPECT_CALL(
      *this->file_storage,
      remove_directory("u-1/ses-1")
    ).WillOnce(::testing::Return(Nyx::Core::Result<void>{}));

    auto result = this->service->delete_session(
      "u-1", "ses-1", this->logger
    );

    ASSERT_TRUE(result.has_value());
  }

  TEST_F(
    ObservationServiceTest, DeleteSessionNotFound
  ) {
    EXPECT_CALL(
      *this->session_repo, find_by_id("ses-99")
    ).WillOnce(::testing::Return(
      std::optional<Nyx::Domain::ObservationSession>{}
    ));

    auto result = this->service->delete_session(
      "u-1", "ses-99", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::ResourceNotFound
    );
  }

  TEST_F(
    ObservationServiceTest,
    DeleteSessionFileCleanupFailsGracefully
  ) {
    this->expect_session_found();
    EXPECT_CALL(*this->session_repo, remove("ses-1"))
      .WillOnce(::testing::Return(Nyx::Core::Result<void>{}));
    EXPECT_CALL(
      *this->file_storage,
      remove_directory("u-1/ses-1")
    ).WillOnce(::testing::Return(std::unexpected(
      Nyx::Core::AppError::internal("disk error")
    )));

    auto result = this->service->delete_session(
      "u-1", "ses-1", this->logger
    );

    ASSERT_TRUE(result.has_value());
  }

  // --- Upload Image ---

  TEST_F(ObservationServiceTest, UploadImageSuccess) {
    auto file_content = std::string("fake-dng-data");
    auto file_data = UploadedFileData{
      .original_filename = "star.dng",
      .data = file_content.data(),
      .length = file_content.size(),
      .mime_type = {},
    };
    auto image = make_image();

    this->expect_session_found();
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("img-1"));
    EXPECT_CALL(
      *this->file_storage,
      save_file("u-1/ses-1", "img-1.dng",
                ::testing::_, ::testing::_)
    ).WillOnce(::testing::Return(
      std::string("u-1/ses-1/img-1.dng")
    ));
    EXPECT_CALL(
      *this->exif_parser,
      parse("u-1/ses-1/img-1.dng")
    ).WillOnce(::testing::Return(ExifData{
      .captured_at = "2026-04-05T00:00:00Z",
      .camera_model = "Pixel 7",
      .exposure_time_seconds = 30.0,
      .iso_speed = 3200,
      .gps_latitude = -20.16,
      .gps_longitude = 57.50,
      .image_width = 4032,
      .image_height = 3024,
    }));
    EXPECT_CALL(
      *this->image_repo, create(::testing::_)
    ).WillOnce(::testing::Return(image));

    auto result = this->service->upload_image(
      "u-1", "ses-1", file_data, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "img-1");
    EXPECT_EQ(result->mime_type, "image/x-adobe-dng");
    EXPECT_EQ(result->camera_model, "Pixel 7");
  }

  TEST_F(
    ObservationServiceTest, UploadImageUnsupportedType
  ) {
    auto file_content = std::string("not-an-image");
    auto file_data = UploadedFileData{
      .original_filename = "readme.txt",
      .data = file_content.data(),
      .length = file_content.size(),
      .mime_type = {},
    };

    this->expect_session_found();

    auto result = this->service->upload_image(
      "u-1", "ses-1", file_data, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::ValidationError
    );
  }

  TEST_F(ObservationServiceTest, UploadImageTooLarge) {
    auto file_data = UploadedFileData{
      .original_filename = "huge.dng",
      .data = nullptr,
      .length = 60 * 1024 * 1024,
      .mime_type = {},
    };

    this->expect_session_found();

    auto result = this->service->upload_image(
      "u-1", "ses-1", file_data, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::ValidationError
    );
  }

  TEST_F(
    ObservationServiceTest,
    UploadImageDbFailCleansUpFile
  ) {
    auto file_content = std::string("fake-dng-data");
    auto file_data = UploadedFileData{
      .original_filename = "star.dng",
      .data = file_content.data(),
      .length = file_content.size(),
      .mime_type = {},
    };

    this->expect_session_found();
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("img-1"));
    EXPECT_CALL(
      *this->file_storage,
      save_file(::testing::_, ::testing::_,
                ::testing::_, ::testing::_)
    ).WillOnce(::testing::Return(
      std::string("u-1/ses-1/img-1.dng")
    ));
    EXPECT_CALL(*this->exif_parser, parse(::testing::_))
      .WillOnce(::testing::Return(ExifData{}));
    EXPECT_CALL(
      *this->image_repo, create(::testing::_)
    ).WillOnce(::testing::Return(std::unexpected(
      Nyx::Core::AppError::internal("DB error")
    )));
    EXPECT_CALL(
      *this->file_storage,
      remove_file("u-1/ses-1/img-1.dng")
    ).WillOnce(::testing::Return(Nyx::Core::Result<void>{}));

    auto result = this->service->upload_image(
      "u-1", "ses-1", file_data, this->logger
    );

    ASSERT_FALSE(result.has_value());
  }

  TEST_F(
    ObservationServiceTest,
    UploadImageSessionNotOwned
  ) {
    auto session = make_session();
    session.user_id = "other-user";

    EXPECT_CALL(
      *this->session_repo, find_by_id("ses-1")
    ).WillOnce(::testing::Return(std::optional{session}));

    auto file_content = std::string("data");
    auto file_data = UploadedFileData{
      .original_filename = "star.dng",
      .data = file_content.data(),
      .length = file_content.size(),
      .mime_type = {},
    };

    auto result = this->service->upload_image(
      "u-1", "ses-1", file_data, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::PermissionDenied
    );
  }

  // --- Delete Image ---

  TEST_F(ObservationServiceTest, DeleteImageSuccess) {
    auto image = make_image();

    this->expect_session_found();
    EXPECT_CALL(*this->image_repo, find_by_id("img-1"))
      .WillOnce(::testing::Return(std::optional{image}));
    EXPECT_CALL(*this->image_repo, remove("img-1"))
      .WillOnce(::testing::Return(Nyx::Core::Result<void>{}));
    EXPECT_CALL(
      *this->file_storage,
      remove_file("u-1/ses-1/img-1.dng")
    ).WillOnce(::testing::Return(Nyx::Core::Result<void>{}));

    auto result = this->service->delete_image(
      "u-1", "ses-1", "img-1", this->logger
    );

    ASSERT_TRUE(result.has_value());
  }

  TEST_F(
    ObservationServiceTest, DeleteImageNotFound
  ) {
    this->expect_session_found();
    EXPECT_CALL(*this->image_repo, find_by_id("img-99"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::ObservationImage>{}
      ));

    auto result = this->service->delete_image(
      "u-1", "ses-1", "img-99", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::ResourceNotFound
    );
  }

  TEST_F(
    ObservationServiceTest, DeleteImageWrongSession
  ) {
    auto image = make_image();
    image.session_id = "other-session";

    this->expect_session_found();
    EXPECT_CALL(*this->image_repo, find_by_id("img-1"))
      .WillOnce(::testing::Return(std::optional{image}));

    auto result = this->service->delete_image(
      "u-1", "ses-1", "img-1", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::ResourceNotFound
    );
  }

  // --- Run Photometry ---

  TEST_F(
    ObservationServiceTest, RunPhotometrySuccess
  ) {
    auto images = std::vector{make_image()};

    this->expect_session_found();
    EXPECT_CALL(
      *this->telescope_repo, find_by_id("tel-1")
    ).WillOnce(::testing::Return(
      std::optional{make_telescope()}
    ));
    EXPECT_CALL(
      *this->camera_repo, find_by_id("cam-1")
    ).WillOnce(::testing::Return(
      std::optional{make_camera()}
    ));
    EXPECT_CALL(
      *this->image_repo, find_by_session_id("ses-1")
    ).WillOnce(::testing::Return(images));
    EXPECT_CALL(
      *this->image_repo,
      update_photometry_status_batch("ses-1", "processing")
    ).WillOnce(::testing::Return(Nyx::Core::Result<void>{}));

    auto request = RunPhotometryRequest{
      .target_x = 150, .target_y = 200,
    };
    auto result = this->service->run_photometry(
      "u-1", "ses-1", request, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->session_id, "ses-1");
    EXPECT_EQ(result->status, "processing");

    std::this_thread::sleep_for(
      std::chrono::milliseconds(100)
    );
  }

  TEST_F(
    ObservationServiceTest, RunPhotometryNoImages
  ) {
    this->expect_session_found();
    EXPECT_CALL(
      *this->telescope_repo, find_by_id("tel-1")
    ).WillOnce(::testing::Return(
      std::optional{make_telescope()}
    ));
    EXPECT_CALL(
      *this->camera_repo, find_by_id("cam-1")
    ).WillOnce(::testing::Return(
      std::optional{make_camera()}
    ));
    EXPECT_CALL(
      *this->image_repo, find_by_session_id("ses-1")
    ).WillOnce(::testing::Return(
      std::vector<Nyx::Domain::ObservationImage>{}
    ));

    auto request = RunPhotometryRequest{
      .target_x = 150, .target_y = 200,
    };
    auto result = this->service->run_photometry(
      "u-1", "ses-1", request, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::ValidationError
    );
  }

  TEST_F(
    ObservationServiceTest,
    RunPhotometryAlreadyInProgress
  ) {
    auto image = make_image();
    image.photometry_status = "processing";

    this->expect_session_found();
    EXPECT_CALL(
      *this->telescope_repo, find_by_id("tel-1")
    ).WillOnce(::testing::Return(
      std::optional{make_telescope()}
    ));
    EXPECT_CALL(
      *this->camera_repo, find_by_id("cam-1")
    ).WillOnce(::testing::Return(
      std::optional{make_camera()}
    ));
    EXPECT_CALL(
      *this->image_repo, find_by_session_id("ses-1")
    ).WillOnce(::testing::Return(std::vector{image}));

    auto request = RunPhotometryRequest{
      .target_x = 150, .target_y = 200,
    };
    auto result = this->service->run_photometry(
      "u-1", "ses-1", request, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::Conflict
    );
  }

  TEST_F(
    ObservationServiceTest,
    RunPhotometrySessionNotOwned
  ) {
    auto session = make_session();
    session.user_id = "other-user";

    EXPECT_CALL(
      *this->session_repo, find_by_id("ses-1")
    ).WillOnce(::testing::Return(std::optional{session}));

    auto request = RunPhotometryRequest{
      .target_x = 150, .target_y = 200,
    };
    auto result = this->service->run_photometry(
      "u-1", "ses-1", request, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::PermissionDenied
    );
  }
} // namespace Nyx::Application::Observation::Tests
