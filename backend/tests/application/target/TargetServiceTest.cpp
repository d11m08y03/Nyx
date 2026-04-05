#include "application/target/TargetService.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace Nyx::Application::Target::Tests {
  class MockMastClient : public IMastClient {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<ResolvedTarget>, resolve_target,
        (const std::string& name), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<MastObservation>>),
        search_tess_timeseries,
        (double ra, double dec, double radius), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<DataProduct>>),
        get_data_products,
        (const std::string& obsid), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<std::string>, download_fits,
        (const std::string& data_uri), (override)
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
        find_by_id, (const std::string& id), (override)
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
        Nyx::Core::Result<Nyx::Domain::TessObservation>, create,
        (const Nyx::Domain::TessObservation& observation), (override)
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
        (Nyx::Core::Result<std::optional<Nyx::Domain::TessObservation>>),
        find_by_id,
        (const std::string& id), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::TessObservation>,
        update_data_uri,
        (const std::string& id, const std::string& data_uri),
        (override)
      );
  };

  class MockUuidGenerator : public Nyx::Core::IUuidGenerator {
    public:
      MOCK_METHOD(std::string, generate, (), (override));
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
        (const std::string& observation_id, bool quality_filter),
        (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<int>, count_by_observation_id,
        (const std::string& observation_id), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, delete_by_observation_id,
        (const std::string& observation_id), (override)
      );
  };

  class MockFitsParser : public IFitsParser {
    public:
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::LightCurvePoint>>),
        parse_light_curve,
        (const std::string& fits_data), (override)
      );
  };

  class TargetServiceTest : public ::testing::Test {
    protected:
      auto SetUp() -> void override {
        this->mast_client = std::make_shared<MockMastClient>();
        this->target_repo = std::make_shared<MockTargetRepository>();
        this->tess_obs_repo =
          std::make_shared<MockTessObservationRepository>();
        this->uuid_gen = std::make_shared<MockUuidGenerator>();
        this->lcp_repo =
          std::make_shared<MockLightCurvePointRepository>();
        this->fits_parser = std::make_shared<MockFitsParser>();
        this->logger = spdlog::default_logger();

        this->service = std::make_unique<TargetService>(
          this->mast_client, this->target_repo,
          this->tess_obs_repo, this->uuid_gen,
          this->lcp_repo, this->fits_parser
        );
      }

      std::shared_ptr<MockMastClient> mast_client;
      std::shared_ptr<MockTargetRepository> target_repo;
      std::shared_ptr<MockTessObservationRepository> tess_obs_repo;
      std::shared_ptr<MockUuidGenerator> uuid_gen;
      std::shared_ptr<MockLightCurvePointRepository> lcp_repo;
      std::shared_ptr<MockFitsParser> fits_parser;
      std::shared_ptr<spdlog::logger> logger;
      std::unique_ptr<TargetService> service;
  };

  // ===== Existing resolve_target tests =====

  TEST_F(TargetServiceTest, ResolveNewTargetWithTessData) {
    auto request = ResolveTargetRequest{.target_name = "Pi Mensae"};

    auto resolved = ResolvedTarget{
      .canonical_name = "pi Men",
      .target_type = std::optional<std::string>("Star"),
      .right_ascension = std::optional<double>(84.291),
      .declination = std::optional<double>(-80.469),
    };

    auto created_target = Nyx::Domain::Target{
      .id = "t-1", .canonical_name = "pi Men",
      .target_type = std::optional<std::string>("Star"),
      .right_ascension = std::optional<double>(84.291),
      .declination = std::optional<double>(-80.469),
    };

    auto mast_observations = std::vector<MastObservation>{
      {.obsid = "obs-1", .cadence_seconds = 120,
       .start_time = 58325.0, .end_time = 58352.0},
      {.obsid = "obs-2", .cadence_seconds = 20,
       .start_time = 58680.0, .end_time = 58707.0},
    };

    auto created_tess_1 = Nyx::Domain::TessObservation{
      .id = "to-1", .target_id = "t-1", .obsid = "obs-1",
      .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = std::nullopt,
    };

    auto created_tess_2 = Nyx::Domain::TessObservation{
      .id = "to-2", .target_id = "t-1", .obsid = "obs-2",
      .cadence_seconds = 20,
      .start_time = 58680.0, .end_time = 58707.0,
      .data_uri = std::nullopt,
    };

    EXPECT_CALL(*this->mast_client, resolve_target("Pi Mensae"))
      .WillOnce(::testing::Return(resolved));
    EXPECT_CALL(*this->target_repo, find_by_canonical_name("pi Men"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>(std::nullopt)
      ));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("t-1"))
      .WillOnce(::testing::Return("to-1"))
      .WillOnce(::testing::Return("to-2"));
    EXPECT_CALL(*this->target_repo, create(::testing::_))
      .WillOnce(::testing::Return(created_target));
    EXPECT_CALL(
      *this->mast_client, search_tess_timeseries(84.291, -80.469, 0.005)
    ).WillOnce(::testing::Return(mast_observations));
    EXPECT_CALL(*this->tess_obs_repo, find_by_obsid("obs-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(std::nullopt)
      ));
    EXPECT_CALL(*this->tess_obs_repo, find_by_obsid("obs-2"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(std::nullopt)
      ));
    EXPECT_CALL(*this->tess_obs_repo, create(::testing::_))
      .WillOnce(::testing::Return(created_tess_1))
      .WillOnce(::testing::Return(created_tess_2));

    auto result = this->service->resolve_target(request, this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "t-1");
    EXPECT_EQ(result->canonical_name, "pi Men");
    EXPECT_EQ(result->tess_observations.size(), 2);
    EXPECT_EQ(result->tess_observations[0].obsid, "obs-1");
    EXPECT_EQ(result->tess_observations[0].cadence_seconds, 120);
    EXPECT_EQ(result->tess_observations[1].obsid, "obs-2");
    EXPECT_EQ(result->tess_observations[1].cadence_seconds, 20);
  }

  TEST_F(TargetServiceTest, ResolveNewTargetNoTessData) {
    auto request = ResolveTargetRequest{.target_name = "M31"};

    auto resolved = ResolvedTarget{
      .canonical_name = "M  31",
      .target_type = std::optional<std::string>("AGN"),
      .right_ascension = std::optional<double>(10.685),
      .declination = std::optional<double>(41.269),
    };

    auto created_target = Nyx::Domain::Target{
      .id = "t-2", .canonical_name = "M  31",
      .target_type = std::optional<std::string>("AGN"),
      .right_ascension = std::optional<double>(10.685),
      .declination = std::optional<double>(41.269),
    };

    EXPECT_CALL(*this->mast_client, resolve_target("M31"))
      .WillOnce(::testing::Return(resolved));
    EXPECT_CALL(*this->target_repo, find_by_canonical_name("M  31"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>(std::nullopt)
      ));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("t-2"));
    EXPECT_CALL(*this->target_repo, create(::testing::_))
      .WillOnce(::testing::Return(created_target));
    EXPECT_CALL(
      *this->mast_client, search_tess_timeseries(10.685, 41.269, 0.005)
    ).WillOnce(::testing::Return(
      std::vector<MastObservation>{}
    ));

    auto result = this->service->resolve_target(request, this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->canonical_name, "M  31");
    EXPECT_TRUE(result->tess_observations.empty());
  }

  TEST_F(TargetServiceTest, ResolveCachedTarget) {
    auto request = ResolveTargetRequest{.target_name = "Pi Mensae"};

    auto resolved = ResolvedTarget{
      .canonical_name = "pi Men",
      .target_type = std::optional<std::string>("Star"),
      .right_ascension = std::optional<double>(84.291),
      .declination = std::optional<double>(-80.469),
    };

    auto existing_target = Nyx::Domain::Target{
      .id = "t-1", .canonical_name = "pi Men",
      .target_type = std::optional<std::string>("Star"),
      .right_ascension = std::optional<double>(84.291),
      .declination = std::optional<double>(-80.469),
    };

    auto stored_observations =
      std::vector<Nyx::Domain::TessObservation>{
        {.id = "to-1", .target_id = "t-1", .obsid = "obs-1",
         .cadence_seconds = 120,
         .start_time = 58325.0, .end_time = 58352.0,
         .data_uri = std::nullopt},
      };

    EXPECT_CALL(*this->mast_client, resolve_target("Pi Mensae"))
      .WillOnce(::testing::Return(resolved));
    EXPECT_CALL(*this->target_repo, find_by_canonical_name("pi Men"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>(existing_target)
      ));
    EXPECT_CALL(*this->tess_obs_repo, find_by_target_id("t-1"))
      .WillOnce(::testing::Return(stored_observations));

    auto result = this->service->resolve_target(request, this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "t-1");
    EXPECT_EQ(result->tess_observations.size(), 1);
    EXPECT_EQ(result->tess_observations[0].obsid, "obs-1");
  }

  TEST_F(TargetServiceTest, ResolveMastNameNotFound) {
    auto request = ResolveTargetRequest{
      .target_name = "notarealstar"
    };

    EXPECT_CALL(*this->mast_client, resolve_target("notarealstar"))
      .WillOnce(::testing::Return(std::unexpected(
        Nyx::Core::AppError::validation(
          "Target could not be resolved"
        )
      )));

    auto result = this->service->resolve_target(request, this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::ValidationError
    );
  }

  TEST_F(TargetServiceTest, ResolveMastApiUnavailable) {
    auto request = ResolveTargetRequest{
      .target_name = "Pi Mensae"
    };

    EXPECT_CALL(*this->mast_client, resolve_target("Pi Mensae"))
      .WillOnce(::testing::Return(std::unexpected(
        Nyx::Core::AppError{
          Nyx::Core::ErrorCode::ExternalServiceError,
          "MAST API request failed", {}
        }
      )));

    auto result = this->service->resolve_target(request, this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::ExternalServiceError
    );
  }

  TEST_F(TargetServiceTest, ResolveTessSearchFails) {
    auto request = ResolveTargetRequest{.target_name = "Pi Mensae"};

    auto resolved = ResolvedTarget{
      .canonical_name = "pi Men",
      .target_type = std::optional<std::string>("Star"),
      .right_ascension = std::optional<double>(84.291),
      .declination = std::optional<double>(-80.469),
    };

    auto created_target = Nyx::Domain::Target{
      .id = "t-1", .canonical_name = "pi Men",
      .target_type = std::optional<std::string>("Star"),
      .right_ascension = std::optional<double>(84.291),
      .declination = std::optional<double>(-80.469),
    };

    EXPECT_CALL(*this->mast_client, resolve_target("Pi Mensae"))
      .WillOnce(::testing::Return(resolved));
    EXPECT_CALL(*this->target_repo, find_by_canonical_name("pi Men"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>(std::nullopt)
      ));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("t-1"));
    EXPECT_CALL(*this->target_repo, create(::testing::_))
      .WillOnce(::testing::Return(created_target));
    EXPECT_CALL(
      *this->mast_client, search_tess_timeseries(84.291, -80.469, 0.005)
    ).WillOnce(::testing::Return(std::unexpected(
      Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "MAST API request failed", {}
      }
    )));

    auto result = this->service->resolve_target(request, this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::ExternalServiceError
    );
  }

  TEST_F(TargetServiceTest, ResolveDbErrorOnTargetCreate) {
    auto request = ResolveTargetRequest{.target_name = "Pi Mensae"};

    auto resolved = ResolvedTarget{
      .canonical_name = "pi Men",
      .target_type = std::optional<std::string>("Star"),
      .right_ascension = std::optional<double>(84.291),
      .declination = std::optional<double>(-80.469),
    };

    EXPECT_CALL(*this->mast_client, resolve_target("Pi Mensae"))
      .WillOnce(::testing::Return(resolved));
    EXPECT_CALL(*this->target_repo, find_by_canonical_name("pi Men"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>(std::nullopt)
      ));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("t-1"));
    EXPECT_CALL(*this->target_repo, create(::testing::_))
      .WillOnce(::testing::Return(std::unexpected(
        Nyx::Core::AppError::internal("DB error")
      )));

    auto result = this->service->resolve_target(request, this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::InternalError
    );
  }

  TEST_F(TargetServiceTest, GetTargetSuccess) {
    auto target = Nyx::Domain::Target{
      .id = "t-1", .canonical_name = "pi Men",
      .target_type = std::optional<std::string>("Star"),
      .right_ascension = std::optional<double>(84.291),
      .declination = std::optional<double>(-80.469),
    };

    auto observations =
      std::vector<Nyx::Domain::TessObservation>{
        {.id = "to-1", .target_id = "t-1", .obsid = "obs-1",
         .cadence_seconds = 120,
         .start_time = 58325.0, .end_time = 58352.0,
         .data_uri = std::nullopt},
      };

    EXPECT_CALL(*this->target_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>(target)
      ));
    EXPECT_CALL(*this->tess_obs_repo, find_by_target_id("t-1"))
      .WillOnce(::testing::Return(observations));

    auto result = this->service->get_target("t-1", this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "t-1");
    EXPECT_EQ(result->canonical_name, "pi Men");
    EXPECT_EQ(result->tess_observations.size(), 1);
  }

  TEST_F(TargetServiceTest, GetTargetNotFound) {
    EXPECT_CALL(*this->target_repo, find_by_id("bad"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>(std::nullopt)
      ));

    auto result = this->service->get_target("bad", this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::ResourceNotFound
    );
  }

  TEST_F(TargetServiceTest, ListTessObservationsSuccess) {
    auto target = Nyx::Domain::Target{
      .id = "t-1", .canonical_name = "pi Men",
      .target_type = std::optional<std::string>("Star"),
      .right_ascension = std::optional<double>(84.291),
      .declination = std::optional<double>(-80.469),
    };

    auto observations =
      std::vector<Nyx::Domain::TessObservation>{
        {.id = "to-1", .target_id = "t-1", .obsid = "obs-1",
         .cadence_seconds = 120,
         .start_time = 58325.0, .end_time = 58352.0,
         .data_uri = std::nullopt},
        {.id = "to-2", .target_id = "t-1", .obsid = "obs-2",
         .cadence_seconds = 20,
         .start_time = 58680.0, .end_time = 58707.0,
         .data_uri = std::nullopt},
      };

    EXPECT_CALL(*this->target_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>(target)
      ));
    EXPECT_CALL(*this->tess_obs_repo, find_by_target_id("t-1"))
      .WillOnce(::testing::Return(observations));

    auto result = this->service->list_tess_observations(
      "t-1", this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 2);
    EXPECT_EQ(result->at(0).obsid, "obs-1");
    EXPECT_EQ(result->at(1).obsid, "obs-2");
  }

  TEST_F(TargetServiceTest, ListTessObservationsEmpty) {
    auto target = Nyx::Domain::Target{
      .id = "t-2", .canonical_name = "M  31",
      .target_type = std::optional<std::string>("AGN"),
      .right_ascension = std::optional<double>(10.685),
      .declination = std::optional<double>(41.269),
    };

    EXPECT_CALL(*this->target_repo, find_by_id("t-2"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>(target)
      ));
    EXPECT_CALL(*this->tess_obs_repo, find_by_target_id("t-2"))
      .WillOnce(::testing::Return(
        std::vector<Nyx::Domain::TessObservation>{}
      ));

    auto result = this->service->list_tess_observations(
      "t-2", this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
  }

  TEST_F(TargetServiceTest, ListTessObservationsTargetNotFound) {
    EXPECT_CALL(*this->target_repo, find_by_id("bad"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>(std::nullopt)
      ));

    auto result = this->service->list_tess_observations(
      "bad", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::ResourceNotFound
    );
  }

  TEST_F(TargetServiceTest, ResolveDuplicateObsidHandling) {
    auto request = ResolveTargetRequest{.target_name = "Pi Mensae"};

    auto resolved = ResolvedTarget{
      .canonical_name = "pi Men",
      .target_type = std::optional<std::string>("Star"),
      .right_ascension = std::optional<double>(84.291),
      .declination = std::optional<double>(-80.469),
    };

    auto created_target = Nyx::Domain::Target{
      .id = "t-1", .canonical_name = "pi Men",
      .target_type = std::optional<std::string>("Star"),
      .right_ascension = std::optional<double>(84.291),
      .declination = std::optional<double>(-80.469),
    };

    auto mast_observations = std::vector<MastObservation>{
      {.obsid = "obs-existing", .cadence_seconds = 120,
       .start_time = 58325.0, .end_time = 58352.0},
      {.obsid = "obs-new", .cadence_seconds = 20,
       .start_time = 58680.0, .end_time = 58707.0},
    };

    auto already_stored = Nyx::Domain::TessObservation{
      .id = "to-existing", .target_id = "t-1",
      .obsid = "obs-existing", .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = std::nullopt,
    };

    auto created_new = Nyx::Domain::TessObservation{
      .id = "to-new", .target_id = "t-1",
      .obsid = "obs-new", .cadence_seconds = 20,
      .start_time = 58680.0, .end_time = 58707.0,
      .data_uri = std::nullopt,
    };

    EXPECT_CALL(*this->mast_client, resolve_target("Pi Mensae"))
      .WillOnce(::testing::Return(resolved));
    EXPECT_CALL(*this->target_repo, find_by_canonical_name("pi Men"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>(std::nullopt)
      ));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("t-1"))
      .WillOnce(::testing::Return("to-new"));
    EXPECT_CALL(*this->target_repo, create(::testing::_))
      .WillOnce(::testing::Return(created_target));
    EXPECT_CALL(
      *this->mast_client, search_tess_timeseries(84.291, -80.469, 0.005)
    ).WillOnce(::testing::Return(mast_observations));
    EXPECT_CALL(*this->tess_obs_repo, find_by_obsid("obs-existing"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(already_stored)
      ));
    EXPECT_CALL(*this->tess_obs_repo, find_by_obsid("obs-new"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(std::nullopt)
      ));
    EXPECT_CALL(*this->tess_obs_repo, create(::testing::_))
      .WillOnce(::testing::Return(created_new));

    auto result = this->service->resolve_target(request, this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->tess_observations.size(), 2);
    EXPECT_EQ(result->tess_observations[0].id, "to-existing");
    EXPECT_EQ(result->tess_observations[1].id, "to-new");
  }

  TEST_F(TargetServiceTest, ResolveDifferentNamesSameTarget) {
    auto request = ResolveTargetRequest{.target_name = "Messier 31"};

    auto resolved = ResolvedTarget{
      .canonical_name = "M  31",
      .target_type = std::optional<std::string>("AGN"),
      .right_ascension = std::optional<double>(10.685),
      .declination = std::optional<double>(41.269),
    };

    auto existing_target = Nyx::Domain::Target{
      .id = "t-2", .canonical_name = "M  31",
      .target_type = std::optional<std::string>("AGN"),
      .right_ascension = std::optional<double>(10.685),
      .declination = std::optional<double>(41.269),
    };

    EXPECT_CALL(*this->mast_client, resolve_target("Messier 31"))
      .WillOnce(::testing::Return(resolved));
    EXPECT_CALL(*this->target_repo, find_by_canonical_name("M  31"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Target>(existing_target)
      ));
    EXPECT_CALL(*this->tess_obs_repo, find_by_target_id("t-2"))
      .WillOnce(::testing::Return(
        std::vector<Nyx::Domain::TessObservation>{}
      ));

    auto result = this->service->resolve_target(request, this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "t-2");
    EXPECT_EQ(result->canonical_name, "M  31");
  }

  // ===== discover_products tests =====

  TEST_F(TargetServiceTest, DiscoverProductsSuccess) {
    auto observation = Nyx::Domain::TessObservation{
      .id = "to-1", .target_id = "t-1", .obsid = "obs-1",
      .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = std::nullopt,
    };

    auto products = std::vector<DataProduct>{
      {.data_uri = "mast:TESS/product/other.fits",
       .description = "Target pixel files",
       .product_type = "SCIENCE",
       .filename = "tess-s0001-obs-1-tp.fits"},
      {.data_uri = "mast:TESS/product/lc.fits",
       .description = "Light curves",
       .product_type = "SCIENCE",
       .filename = "tess-s0001-obs-1-lc.fits"},
    };

    auto updated_observation = Nyx::Domain::TessObservation{
      .id = "to-1", .target_id = "t-1", .obsid = "obs-1",
      .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = "mast:TESS/product/lc.fits",
    };

    EXPECT_CALL(*this->tess_obs_repo, find_by_id("to-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(observation)
      ));
    EXPECT_CALL(*this->mast_client, get_data_products("obs-1"))
      .WillOnce(::testing::Return(products));
    EXPECT_CALL(
      *this->tess_obs_repo,
      update_data_uri("to-1", "mast:TESS/product/lc.fits")
    ).WillOnce(::testing::Return(updated_observation));

    auto result = this->service->discover_products(
      "to-1", this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "to-1");
    EXPECT_EQ(result->data_uri.value(), "mast:TESS/product/lc.fits");
  }

  TEST_F(TargetServiceTest, DiscoverProductsAlreadyHasUri) {
    auto observation = Nyx::Domain::TessObservation{
      .id = "to-1", .target_id = "t-1", .obsid = "obs-1",
      .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = "mast:TESS/product/lc.fits",
    };

    EXPECT_CALL(*this->tess_obs_repo, find_by_id("to-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(observation)
      ));

    auto result = this->service->discover_products(
      "to-1", this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->data_uri.value(), "mast:TESS/product/lc.fits");
  }

  TEST_F(TargetServiceTest, DiscoverProductsNotFound) {
    EXPECT_CALL(*this->tess_obs_repo, find_by_id("bad"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(std::nullopt)
      ));

    auto result = this->service->discover_products(
      "bad", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::ResourceNotFound
    );
  }

  TEST_F(TargetServiceTest, DiscoverProductsNoLcFile) {
    auto observation = Nyx::Domain::TessObservation{
      .id = "to-1", .target_id = "t-1", .obsid = "obs-1",
      .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = std::nullopt,
    };

    auto products = std::vector<DataProduct>{
      {.data_uri = "mast:TESS/product/tp.fits",
       .description = "Target pixel files",
       .product_type = "SCIENCE",
       .filename = "tess-s0001-obs-1-tp.fits"},
    };

    EXPECT_CALL(*this->tess_obs_repo, find_by_id("to-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(observation)
      ));
    EXPECT_CALL(*this->mast_client, get_data_products("obs-1"))
      .WillOnce(::testing::Return(products));

    auto result = this->service->discover_products(
      "to-1", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::ResourceNotFound
    );
  }

  TEST_F(TargetServiceTest, DiscoverProductsMastFailure) {
    auto observation = Nyx::Domain::TessObservation{
      .id = "to-1", .target_id = "t-1", .obsid = "obs-1",
      .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = std::nullopt,
    };

    EXPECT_CALL(*this->tess_obs_repo, find_by_id("to-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(observation)
      ));
    EXPECT_CALL(*this->mast_client, get_data_products("obs-1"))
      .WillOnce(::testing::Return(std::unexpected(
        Nyx::Core::AppError{
          Nyx::Core::ErrorCode::ExternalServiceError,
          "MAST API request failed", {}
        }
      )));

    auto result = this->service->discover_products(
      "to-1", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::ExternalServiceError
    );
  }

  // ===== fetch_light_curve tests =====

  TEST_F(TargetServiceTest, FetchLightCurveSuccess) {
    auto observation = Nyx::Domain::TessObservation{
      .id = "to-1", .target_id = "t-1", .obsid = "obs-1",
      .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = "mast:TESS/product/lc.fits",
    };

    auto parsed_points = std::vector<Nyx::Domain::LightCurvePoint>{
      {.id = 0, .tess_observation_id = "",
       .time = 1325.5, .pdcsap_flux = 100.0f,
       .pdcsap_flux_err = 0.5f, .sap_flux = 101.0f,
       .quality = 0},
      {.id = 0, .tess_observation_id = "",
       .time = 1325.6, .pdcsap_flux = 99.5f,
       .pdcsap_flux_err = 0.5f, .sap_flux = 100.5f,
       .quality = 0},
    };

    EXPECT_CALL(*this->tess_obs_repo, find_by_id("to-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(observation)
      ));
    EXPECT_CALL(*this->lcp_repo, count_by_observation_id("to-1"))
      .WillOnce(::testing::Return(0));
    EXPECT_CALL(
      *this->mast_client,
      download_fits("mast:TESS/product/lc.fits")
    ).WillOnce(::testing::Return(std::string("fake-fits-data")));
    EXPECT_CALL(
      *this->fits_parser, parse_light_curve("fake-fits-data")
    ).WillOnce(::testing::Return(parsed_points));
    EXPECT_CALL(*this->lcp_repo, bulk_create("to-1", ::testing::_))
      .WillOnce(::testing::Return(2));

    auto result = this->service->fetch_light_curve(
      "to-1", this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->tess_observation_id, "to-1");
    EXPECT_EQ(result->obsid, "obs-1");
    EXPECT_EQ(result->point_count, 2);
    EXPECT_DOUBLE_EQ(result->time_min, 1325.5);
    EXPECT_DOUBLE_EQ(result->time_max, 1325.6);
  }

  TEST_F(TargetServiceTest, FetchLightCurveNoDataUri) {
    auto observation = Nyx::Domain::TessObservation{
      .id = "to-1", .target_id = "t-1", .obsid = "obs-1",
      .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = std::nullopt,
    };

    EXPECT_CALL(*this->tess_obs_repo, find_by_id("to-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(observation)
      ));

    auto result = this->service->fetch_light_curve(
      "to-1", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code, Nyx::Core::ErrorCode::ValidationError
    );
  }

  TEST_F(TargetServiceTest, FetchLightCurveAlreadyFetched) {
    auto observation = Nyx::Domain::TessObservation{
      .id = "to-1", .target_id = "t-1", .obsid = "obs-1",
      .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = "mast:TESS/product/lc.fits",
    };

    auto stored_points = std::vector<Nyx::Domain::LightCurvePoint>{
      {.id = 1, .tess_observation_id = "to-1",
       .time = 1325.5, .pdcsap_flux = 100.0f,
       .pdcsap_flux_err = 0.5f, .sap_flux = 101.0f,
       .quality = 0},
      {.id = 2, .tess_observation_id = "to-1",
       .time = 1325.6, .pdcsap_flux = 99.5f,
       .pdcsap_flux_err = 0.5f, .sap_flux = 100.5f,
       .quality = 0},
    };

    EXPECT_CALL(*this->tess_obs_repo, find_by_id("to-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(observation)
      ));
    EXPECT_CALL(*this->lcp_repo, count_by_observation_id("to-1"))
      .WillOnce(::testing::Return(2));
    EXPECT_CALL(
      *this->lcp_repo, find_by_observation_id("to-1", false)
    ).WillOnce(::testing::Return(stored_points));

    auto result = this->service->fetch_light_curve(
      "to-1", this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->point_count, 2);
    EXPECT_DOUBLE_EQ(result->time_min, 1325.5);
    EXPECT_DOUBLE_EQ(result->time_max, 1325.6);
  }

  TEST_F(TargetServiceTest, FetchLightCurveDownloadFailure) {
    auto observation = Nyx::Domain::TessObservation{
      .id = "to-1", .target_id = "t-1", .obsid = "obs-1",
      .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = "mast:TESS/product/lc.fits",
    };

    EXPECT_CALL(*this->tess_obs_repo, find_by_id("to-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(observation)
      ));
    EXPECT_CALL(*this->lcp_repo, count_by_observation_id("to-1"))
      .WillOnce(::testing::Return(0));
    EXPECT_CALL(
      *this->mast_client,
      download_fits("mast:TESS/product/lc.fits")
    ).WillOnce(::testing::Return(std::unexpected(
      Nyx::Core::AppError{
        Nyx::Core::ErrorCode::ExternalServiceError,
        "MAST download failed", {}
      }
    )));

    auto result = this->service->fetch_light_curve(
      "to-1", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
      result.error().code,
      Nyx::Core::ErrorCode::ExternalServiceError
    );
  }

  // ===== get_light_curve tests =====

  TEST_F(TargetServiceTest, GetLightCurveSuccess) {
    auto observation = Nyx::Domain::TessObservation{
      .id = "to-1", .target_id = "t-1", .obsid = "obs-1",
      .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = "mast:TESS/product/lc.fits",
    };

    auto points = std::vector<Nyx::Domain::LightCurvePoint>{
      {.id = 1, .tess_observation_id = "to-1",
       .time = 1325.5, .pdcsap_flux = 100.0f,
       .pdcsap_flux_err = 0.5f, .sap_flux = 101.0f,
       .quality = 0},
      {.id = 2, .tess_observation_id = "to-1",
       .time = 1325.6, .pdcsap_flux = 99.5f,
       .pdcsap_flux_err = 0.5f, .sap_flux = 100.5f,
       .quality = 0},
    };

    EXPECT_CALL(*this->tess_obs_repo, find_by_id("to-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(observation)
      ));
    EXPECT_CALL(
      *this->lcp_repo, find_by_observation_id("to-1", false)
    ).WillOnce(::testing::Return(points));

    auto result = this->service->get_light_curve(
      "to-1", false, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->tess_observation_id, "to-1");
    EXPECT_EQ(result->obsid, "obs-1");
    EXPECT_EQ(result->point_count, 2);
    EXPECT_EQ(result->points.size(), 2);
    EXPECT_DOUBLE_EQ(result->points[0].time, 1325.5);
  }

  TEST_F(TargetServiceTest, GetLightCurveWithQualityFilter) {
    auto observation = Nyx::Domain::TessObservation{
      .id = "to-1", .target_id = "t-1", .obsid = "obs-1",
      .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = "mast:TESS/product/lc.fits",
    };

    auto filtered_points =
      std::vector<Nyx::Domain::LightCurvePoint>{
        {.id = 1, .tess_observation_id = "to-1",
         .time = 1325.5, .pdcsap_flux = 100.0f,
         .pdcsap_flux_err = 0.5f, .sap_flux = 101.0f,
         .quality = 0},
      };

    EXPECT_CALL(*this->tess_obs_repo, find_by_id("to-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(observation)
      ));
    EXPECT_CALL(
      *this->lcp_repo, find_by_observation_id("to-1", true)
    ).WillOnce(::testing::Return(filtered_points));

    auto result = this->service->get_light_curve(
      "to-1", true, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->point_count, 1);
  }

  TEST_F(TargetServiceTest, GetLightCurveEmpty) {
    auto observation = Nyx::Domain::TessObservation{
      .id = "to-1", .target_id = "t-1", .obsid = "obs-1",
      .cadence_seconds = 120,
      .start_time = 58325.0, .end_time = 58352.0,
      .data_uri = std::nullopt,
    };

    EXPECT_CALL(*this->tess_obs_repo, find_by_id("to-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::TessObservation>(observation)
      ));
    EXPECT_CALL(
      *this->lcp_repo, find_by_observation_id("to-1", false)
    ).WillOnce(::testing::Return(
      std::vector<Nyx::Domain::LightCurvePoint>{}
    ));

    auto result = this->service->get_light_curve(
      "to-1", false, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->point_count, 0);
    EXPECT_TRUE(result->points.empty());
  }
} // namespace Nyx::Application::Target::Tests
