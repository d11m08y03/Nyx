#include "application/target/LightCurveComparisonService.hpp"
#include "core/util/TimeConversion.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace Nyx::Application::Target::Tests {
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
        (const std::string& id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::Target>>),
        find_by_canonical_name,
        (const std::string& canonical_name), (override)
      );
  };

  class MockTessObservationRepository
    : public Nyx::Domain::ITessObservationRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::TessObservation>,
        create,
        (const Nyx::Domain::TessObservation& obs),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::TessObservation>>),
        find_by_target_id,
        (const std::string& target_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::TessObservation>>),
        find_by_obsid,
        (const std::string& obsid), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::unordered_set<std::string>>),
        find_existing_obsids,
        (const std::vector<std::string>& obsids),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::TessObservation>>),
        bulk_create,
        (const std::vector<Nyx::Domain::TessObservation>& obs),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::TessObservation>>),
        find_by_id,
        (const std::string& id), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::TessObservation>,
        update_data_uri,
        (const std::string& id,
         const std::string& data_uri),
        (override)
      );
  };

  class MockLightCurvePointRepository
    : public Nyx::Domain::ILightCurvePointRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<int>, bulk_create,
        (const std::string& observation_id,
         const std::vector<Nyx::Domain::LightCurvePoint>& points),
        (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::LightCurvePoint>>),
        find_by_observation_id,
        (const std::string& observation_id,
         bool quality_filter),
        (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<std::string>,
        find_by_observation_id_as_json,
        (const std::string& observation_id,
         bool quality_filter),
        (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<int>,
        count_by_observation_id,
        (const std::string& observation_id),
        (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>,
        delete_by_observation_id,
        (const std::string& observation_id),
        (override)
      );
  };

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
        (const std::string& user_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::ObservationSession>>),
        find_by_id,
        (const std::string& id), (override)
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
        (const std::string& session_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::ObservationImage>>),
        find_by_id,
        (const std::string& id), (override)
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

  // --- Test data helpers ---

  static auto make_target() -> Nyx::Domain::Target {
    return {
      .id = "t-1", .canonical_name = "pi Men",
      .target_type = "star",
      .right_ascension = 84.29,
      .declination = -80.47,
    };
  }

  static auto make_tess_observation()
    -> Nyx::Domain::TessObservation {
    return {
      .id = "to-1", .target_id = "t-1",
      .obsid = "tess2019006130736-s0001",
      .cadence_seconds = 120,
      .start_time = 1325.29, .end_time = 1353.18,
      .data_uri = "mast:TESS/product/lc.fits",
    };
  }

  static auto make_tess_points()
    -> std::vector<Nyx::Domain::LightCurvePoint> {
    return {
      {.id = 1, .tess_observation_id = "to-1",
       .time = 1325.5, .pdcsap_flux = 1.0002f,
       .pdcsap_flux_err = 0.0001f,
       .sap_flux = 1.0005f, .quality = 0},
      {.id = 2, .tess_observation_id = "to-1",
       .time = 1325.6, .pdcsap_flux = 0.9998f,
       .pdcsap_flux_err = 0.0001f,
       .sap_flux = 1.0001f, .quality = 0},
    };
  }

  static auto make_session()
    -> Nyx::Domain::ObservationSession {
    return {
      .id = "ses-1", .user_id = "u-1",
      .target_id = "t-1",
      .telescope_id = "tel-1",
      .camera_id = "cam-1", .mount_id = "mnt-1",
      .location_id = "loc-1",
      .filter_id = std::nullopt,
      .notes = std::nullopt,
      .created_at = "2026-04-05T00:00:00Z",
      .updated_at = "2026-04-05T00:00:00Z",
    };
  }

  static auto make_completed_image()
    -> Nyx::Domain::ObservationImage {
    return {
      .id = "img-1", .session_id = "ses-1",
      .filename = "img-1.dng",
      .original_filename = "star.dng",
      .file_path = "u-1/ses-1/img-1.dng",
      .file_size_bytes = 1024,
      .mime_type = "image/x-adobe-dng",
      .captured_at = "2026-04-05T22:30:00Z",
      .camera_model = "Pixel 7",
      .exposure_time_seconds = 30.0,
      .iso_speed = 3200,
      .gps_latitude = -20.16,
      .gps_longitude = 57.50,
      .image_width = 4032, .image_height = 3024,
      .target_x = 150, .target_y = 200,
      .raw_flux = 50000.0,
      .raw_flux_error = 224.0,
      .relative_flux = 0.985,
      .relative_flux_error = 0.012,
      .photometry_status = "completed",
      .photometry_error_message = std::nullopt,
      .created_at = "2026-04-05T22:30:00Z",
    };
  }

  // --- Fixture ---

  class LightCurveComparisonServiceTest
    : public ::testing::Test {
    protected:
      auto SetUp() -> void override {
        this->target_repo =
          std::make_shared<MockTargetRepository>();
        this->tess_obs_repo =
          std::make_shared<
            MockTessObservationRepository
          >();
        this->lc_point_repo =
          std::make_shared<
            MockLightCurvePointRepository
          >();
        this->session_repo =
          std::make_shared<MockSessionRepository>();
        this->image_repo =
          std::make_shared<MockImageRepository>();
        this->logger = spdlog::default_logger();

        this->service = std::make_unique<
          LightCurveComparisonService
        >(
          this->target_repo, this->tess_obs_repo,
          this->lc_point_repo, this->session_repo,
          this->image_repo
        );
      }

      std::shared_ptr<MockTargetRepository> target_repo;
      std::shared_ptr<MockTessObservationRepository>
        tess_obs_repo;
      std::shared_ptr<MockLightCurvePointRepository>
        lc_point_repo;
      std::shared_ptr<MockSessionRepository> session_repo;
      std::shared_ptr<MockImageRepository> image_repo;
      std::shared_ptr<spdlog::logger> logger;
      std::unique_ptr<LightCurveComparisonService>
        service;
  };

  // --- Tests ---

  TEST_F(
    LightCurveComparisonServiceTest, TargetNotFound
  ) {
    EXPECT_CALL(*this->target_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>{}
      ));

    auto result = this->service->get_comparison(
      "u-1", "t-1", "to-1", false, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::ResourceNotFound
    );
  }

  TEST_F(
    LightCurveComparisonServiceTest,
    TessObservationNotFound
  ) {
    EXPECT_CALL(*this->target_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional{make_target()}
      ));
    EXPECT_CALL(
      *this->tess_obs_repo, find_by_id("to-99")
    ).WillOnce(::testing::Return(
      std::optional<Nyx::Domain::TessObservation>{}
    ));

    auto result = this->service->get_comparison(
      "u-1", "t-1", "to-99", false, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::ResourceNotFound
    );
  }

  TEST_F(
    LightCurveComparisonServiceTest,
    TessObservationWrongTarget
  ) {
    auto wrong_obs = make_tess_observation();
    wrong_obs.target_id = "other-target";

    EXPECT_CALL(*this->target_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional{make_target()}
      ));
    EXPECT_CALL(
      *this->tess_obs_repo, find_by_id("to-1")
    ).WillOnce(::testing::Return(
      std::optional{wrong_obs}
    ));

    auto result = this->service->get_comparison(
      "u-1", "t-1", "to-1", false, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::ResourceNotFound
    );
  }

  TEST_F(
    LightCurveComparisonServiceTest,
    NoUserSessionsReturnsEmptyUserData
  ) {
    EXPECT_CALL(*this->target_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional{make_target()}
      ));
    EXPECT_CALL(
      *this->tess_obs_repo, find_by_id("to-1")
    ).WillOnce(::testing::Return(
      std::optional{make_tess_observation()}
    ));
    EXPECT_CALL(
      *this->lc_point_repo,
      find_by_observation_id("to-1", false)
    ).WillOnce(::testing::Return(make_tess_points()));
    EXPECT_CALL(
      *this->session_repo,
      find_by_user_id_and_target_id("u-1", "t-1")
    ).WillOnce(::testing::Return(
      std::vector<Nyx::Domain::ObservationSession>{}
    ));

    auto result = this->service->get_comparison(
      "u-1", "t-1", "to-1", false, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->tess.point_count, 2);
    EXPECT_EQ(
      result->user_observations.point_count, 0
    );
    EXPECT_EQ(
      result->user_observations.session_count, 0
    );
  }

  TEST_F(
    LightCurveComparisonServiceTest,
    ExcludesNonCompletedPhotometry
  ) {
    auto processing_image = make_completed_image();
    processing_image.id = "img-2";
    processing_image.photometry_status = "processing";
    processing_image.relative_flux = std::nullopt;

    auto failed_image = make_completed_image();
    failed_image.id = "img-3";
    failed_image.photometry_status = "failed";
    failed_image.relative_flux = std::nullopt;

    auto no_status_image = make_completed_image();
    no_status_image.id = "img-4";
    no_status_image.photometry_status = std::nullopt;
    no_status_image.relative_flux = std::nullopt;

    EXPECT_CALL(*this->target_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional{make_target()}
      ));
    EXPECT_CALL(
      *this->tess_obs_repo, find_by_id("to-1")
    ).WillOnce(::testing::Return(
      std::optional{make_tess_observation()}
    ));
    EXPECT_CALL(
      *this->lc_point_repo,
      find_by_observation_id("to-1", false)
    ).WillOnce(::testing::Return(make_tess_points()));
    EXPECT_CALL(
      *this->session_repo,
      find_by_user_id_and_target_id("u-1", "t-1")
    ).WillOnce(::testing::Return(
      std::vector{make_session()}
    ));
    EXPECT_CALL(
      *this->image_repo,
      find_by_session_id("ses-1")
    ).WillOnce(::testing::Return(std::vector{
      make_completed_image(), processing_image,
      failed_image, no_status_image,
    }));

    auto result = this->service->get_comparison(
      "u-1", "t-1", "to-1", false, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(
      result->user_observations.point_count, 1
    );
    EXPECT_EQ(
      result->user_observations.session_count, 1
    );
  }

  TEST_F(
    LightCurveComparisonServiceTest,
    ExcludesImagesWithoutCapturedAt
  ) {
    auto no_timestamp = make_completed_image();
    no_timestamp.captured_at = std::nullopt;

    EXPECT_CALL(*this->target_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional{make_target()}
      ));
    EXPECT_CALL(
      *this->tess_obs_repo, find_by_id("to-1")
    ).WillOnce(::testing::Return(
      std::optional{make_tess_observation()}
    ));
    EXPECT_CALL(
      *this->lc_point_repo,
      find_by_observation_id("to-1", false)
    ).WillOnce(::testing::Return(make_tess_points()));
    EXPECT_CALL(
      *this->session_repo,
      find_by_user_id_and_target_id("u-1", "t-1")
    ).WillOnce(::testing::Return(
      std::vector{make_session()}
    ));
    EXPECT_CALL(
      *this->image_repo,
      find_by_session_id("ses-1")
    ).WillOnce(::testing::Return(
      std::vector{no_timestamp}
    ));

    auto result = this->service->get_comparison(
      "u-1", "t-1", "to-1", false, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(
      result->user_observations.point_count, 0
    );
  }

  TEST_F(
    LightCurveComparisonServiceTest,
    TessTimesOffsetByBtjd
  ) {
    EXPECT_CALL(*this->target_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional{make_target()}
      ));
    EXPECT_CALL(
      *this->tess_obs_repo, find_by_id("to-1")
    ).WillOnce(::testing::Return(
      std::optional{make_tess_observation()}
    ));
    EXPECT_CALL(
      *this->lc_point_repo,
      find_by_observation_id("to-1", false)
    ).WillOnce(::testing::Return(make_tess_points()));
    EXPECT_CALL(
      *this->session_repo,
      find_by_user_id_and_target_id("u-1", "t-1")
    ).WillOnce(::testing::Return(
      std::vector<Nyx::Domain::ObservationSession>{}
    ));

    auto result = this->service->get_comparison(
      "u-1", "t-1", "to-1", false, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(
      result->tess.points[0].time,
      1325.5 + 2457000.0, 0.001
    );
    EXPECT_NEAR(
      result->tess.points[1].time,
      1325.6 + 2457000.0, 0.001
    );
  }

  TEST_F(
    LightCurveComparisonServiceTest,
    TimeConversionProducesCorrectJd
  ) {
    // 2026-04-05T12:00:00Z = JD 2461402.0
    auto jd = Nyx::Core::TimeConversion::iso8601_to_jd(
      "2026-04-05T12:00:00Z"
    );

    ASSERT_TRUE(jd.has_value());
    EXPECT_NEAR(jd.value(), 2461136.0, 0.001);
  }

  TEST_F(
    LightCurveComparisonServiceTest,
    TimeConversionHandlesPostgresFormat
  ) {
    auto jd = Nyx::Core::TimeConversion::iso8601_to_jd(
      "2026-04-05 12:00:00+00"
    );

    ASSERT_TRUE(jd.has_value());
    EXPECT_NEAR(jd.value(), 2461136.0, 0.001);
  }

  TEST_F(
    LightCurveComparisonServiceTest,
    HappyPathBothDatasets
  ) {
    EXPECT_CALL(*this->target_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional{make_target()}
      ));
    EXPECT_CALL(
      *this->tess_obs_repo, find_by_id("to-1")
    ).WillOnce(::testing::Return(
      std::optional{make_tess_observation()}
    ));
    EXPECT_CALL(
      *this->lc_point_repo,
      find_by_observation_id("to-1", false)
    ).WillOnce(::testing::Return(make_tess_points()));
    EXPECT_CALL(
      *this->session_repo,
      find_by_user_id_and_target_id("u-1", "t-1")
    ).WillOnce(::testing::Return(
      std::vector{make_session()}
    ));
    EXPECT_CALL(
      *this->image_repo,
      find_by_session_id("ses-1")
    ).WillOnce(::testing::Return(
      std::vector{make_completed_image()}
    ));

    auto result = this->service->get_comparison(
      "u-1", "t-1", "to-1", false, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->target.id, "t-1");
    EXPECT_EQ(
      result->target.canonical_name, "pi Men"
    );
    EXPECT_EQ(result->time_system, "bjd_tdb");

    EXPECT_EQ(
      result->tess.tess_observation_id, "to-1"
    );
    EXPECT_EQ(result->tess.cadence_seconds, 120);
    EXPECT_EQ(result->tess.point_count, 2);

    EXPECT_EQ(
      result->user_observations.session_count, 1
    );
    EXPECT_EQ(
      result->user_observations.point_count, 1
    );
    EXPECT_NEAR(
      result->user_observations.points[0]
        .relative_flux,
      0.985, 0.001
    );
    EXPECT_EQ(
      result->user_observations.points[0].session_id,
      "ses-1"
    );
  }

  TEST_F(
    LightCurveComparisonServiceTest,
    UserPointsSortedByTime
  ) {
    auto later_image = make_completed_image();
    later_image.id = "img-2";
    later_image.captured_at = "2026-04-06T22:30:00Z";

    auto earlier_image = make_completed_image();
    earlier_image.id = "img-3";
    earlier_image.captured_at = "2026-04-04T22:30:00Z";

    EXPECT_CALL(*this->target_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional{make_target()}
      ));
    EXPECT_CALL(
      *this->tess_obs_repo, find_by_id("to-1")
    ).WillOnce(::testing::Return(
      std::optional{make_tess_observation()}
    ));
    EXPECT_CALL(
      *this->lc_point_repo,
      find_by_observation_id("to-1", false)
    ).WillOnce(::testing::Return(make_tess_points()));
    EXPECT_CALL(
      *this->session_repo,
      find_by_user_id_and_target_id("u-1", "t-1")
    ).WillOnce(::testing::Return(
      std::vector{make_session()}
    ));
    EXPECT_CALL(
      *this->image_repo,
      find_by_session_id("ses-1")
    ).WillOnce(::testing::Return(
      std::vector{later_image, earlier_image}
    ));

    auto result = this->service->get_comparison(
      "u-1", "t-1", "to-1", false, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(
      result->user_observations.point_count, 2
    );
    EXPECT_LT(
      result->user_observations.points[0].time,
      result->user_observations.points[1].time
    );
  }
} // namespace Nyx::Application::Target::Tests
