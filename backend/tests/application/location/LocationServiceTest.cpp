#include "application/location/LocationService.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace Nyx::Application::Location::Tests {
  class MockObservingLocationRepository
    : public Nyx::Domain::IObservingLocationRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::ObservingLocation>, create,
        (const Nyx::Domain::ObservingLocation& location), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::ObservingLocation>>),
        find_by_user_id, (const std::string& user_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::ObservingLocation>>),
        find_by_id, (const std::string& id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::ObservingLocation>>),
        find_by_user_id_and_name,
        (const std::string& user_id, const std::string& name), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::ObservingLocation>, update,
        (const Nyx::Domain::ObservingLocation& location), (override)
      );
      MOCK_METHOD(
        Nyx::Core::Result<void>, remove,
        (const std::string& id), (override)
      );
  };

  class MockUuidGenerator : public Nyx::Core::IUuidGenerator {
    public:
      MOCK_METHOD(std::string, generate, (), (override));
  };

  class LocationServiceTest : public ::testing::Test {
    protected:
      auto SetUp() -> void override {
        this->location_repo =
          std::make_shared<MockObservingLocationRepository>();
        this->uuid_gen = std::make_shared<MockUuidGenerator>();
        this->logger = spdlog::default_logger();

        this->service = std::make_unique<LocationService>(
          this->location_repo, this->uuid_gen
        );
      }

      std::shared_ptr<MockObservingLocationRepository> location_repo;
      std::shared_ptr<MockUuidGenerator> uuid_gen;
      std::shared_ptr<spdlog::logger> logger;
      std::unique_ptr<LocationService> service;
  };

  TEST_F(LocationServiceTest, CreateLocationSuccess) {
    auto request = CreateLocationRequest{
      .name = "Backyard", .latitude = -20.2, .longitude = 57.5,
      .bortle_class = std::optional<int16_t>(5),
    };

    auto created = Nyx::Domain::ObservingLocation{
      .id = "loc-1", .user_id = "u-1", .name = "Backyard",
      .latitude = -20.2, .longitude = 57.5,
      .bortle_class = std::optional<int16_t>(5),
    };

    EXPECT_CALL(
      *this->location_repo, find_by_user_id_and_name("u-1", "Backyard")
    ).WillOnce(::testing::Return(
      std::optional<Nyx::Domain::ObservingLocation>(std::nullopt)
    ));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("loc-1"));
    EXPECT_CALL(*this->location_repo, create(::testing::_))
      .WillOnce(::testing::Return(created));

    auto result = this->service->create_location(
      "u-1", request, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "loc-1");
    EXPECT_EQ(result->name, "Backyard");
    EXPECT_DOUBLE_EQ(result->latitude, -20.2);
    EXPECT_DOUBLE_EQ(result->longitude, 57.5);
    ASSERT_TRUE(result->bortle_class.has_value());
    EXPECT_EQ(result->bortle_class.value(), 5);
  }

  TEST_F(LocationServiceTest, CreateLocationWithoutBortleClass) {
    auto request = CreateLocationRequest{
      .name = "Remote Site", .latitude = -20.5, .longitude = 57.7,
      .bortle_class = std::nullopt,
    };

    auto created = Nyx::Domain::ObservingLocation{
      .id = "loc-2", .user_id = "u-1", .name = "Remote Site",
      .latitude = -20.5, .longitude = 57.7,
      .bortle_class = std::nullopt,
    };

    EXPECT_CALL(
      *this->location_repo,
      find_by_user_id_and_name("u-1", "Remote Site")
    ).WillOnce(::testing::Return(
      std::optional<Nyx::Domain::ObservingLocation>(std::nullopt)
    ));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("loc-2"));
    EXPECT_CALL(*this->location_repo, create(::testing::_))
      .WillOnce(::testing::Return(created));

    auto result = this->service->create_location(
      "u-1", request, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->bortle_class.has_value());
  }

  TEST_F(LocationServiceTest, CreateLocationDatabaseError) {
    auto request = CreateLocationRequest{
      .name = "Backyard", .latitude = -20.2, .longitude = 57.5,
      .bortle_class = std::nullopt,
    };

    EXPECT_CALL(
      *this->location_repo, find_by_user_id_and_name("u-1", "Backyard")
    ).WillOnce(::testing::Return(
      std::optional<Nyx::Domain::ObservingLocation>(std::nullopt)
    ));
    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("loc-1"));
    EXPECT_CALL(*this->location_repo, create(::testing::_))
      .WillOnce(::testing::Return(
        std::unexpected(Nyx::Core::AppError::internal("DB error"))
      ));

    auto result = this->service->create_location(
      "u-1", request, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::InternalError);
  }

  TEST_F(LocationServiceTest, CreateLocationDuplicateNameConflict) {
    auto request = CreateLocationRequest{
      .name = "Backyard", .latitude = -20.2, .longitude = 57.5,
      .bortle_class = std::nullopt,
    };

    auto existing_location = Nyx::Domain::ObservingLocation{
      .id = "loc-existing", .user_id = "u-1", .name = "Backyard",
      .latitude = -20.0, .longitude = 57.0,
      .bortle_class = std::nullopt,
    };

    EXPECT_CALL(
      *this->location_repo, find_by_user_id_and_name("u-1", "Backyard")
    ).WillOnce(::testing::Return(
      std::optional<Nyx::Domain::ObservingLocation>(existing_location)
    ));

    auto result = this->service->create_location(
      "u-1", request, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::Conflict);
  }

  TEST_F(LocationServiceTest, ListLocationsSuccess) {
    auto locations = std::vector<Nyx::Domain::ObservingLocation>{
      {.id = "loc-1", .user_id = "u-1", .name = "Backyard",
       .latitude = -20.2, .longitude = 57.5,
       .bortle_class = std::optional<int16_t>(5)},
      {.id = "loc-2", .user_id = "u-1", .name = "Dark Site",
       .latitude = -20.5, .longitude = 57.7,
       .bortle_class = std::optional<int16_t>(2)},
    };

    EXPECT_CALL(*this->location_repo, find_by_user_id("u-1"))
      .WillOnce(::testing::Return(locations));

    auto result = this->service->list_locations("u-1", this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 2);
    EXPECT_EQ(result->at(0).name, "Backyard");
    EXPECT_EQ(result->at(1).name, "Dark Site");
  }

  TEST_F(LocationServiceTest, GetLocationSuccess) {
    auto location = Nyx::Domain::ObservingLocation{
      .id = "loc-1", .user_id = "u-1", .name = "Backyard",
      .latitude = -20.2, .longitude = 57.5,
      .bortle_class = std::optional<int16_t>(5),
    };

    EXPECT_CALL(*this->location_repo, find_by_id("loc-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::ObservingLocation>(location)
      ));

    auto result = this->service->get_location(
      "u-1", "loc-1", this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "loc-1");
  }

  TEST_F(LocationServiceTest, GetLocationNotFound) {
    EXPECT_CALL(*this->location_repo, find_by_id("bad"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::ObservingLocation>(std::nullopt)
      ));

    auto result = this->service->get_location(
      "u-1", "bad", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::ResourceNotFound);
  }

  TEST_F(LocationServiceTest, GetLocationOwnershipDenied) {
    auto location = Nyx::Domain::ObservingLocation{
      .id = "loc-1", .user_id = "other-user", .name = "Backyard",
      .latitude = -20.2, .longitude = 57.5,
      .bortle_class = std::nullopt,
    };

    EXPECT_CALL(*this->location_repo, find_by_id("loc-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::ObservingLocation>(location)
      ));

    auto result = this->service->get_location(
      "u-1", "loc-1", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::PermissionDenied);
  }

  TEST_F(LocationServiceTest, UpdateLocationSuccess) {
    auto existing = Nyx::Domain::ObservingLocation{
      .id = "loc-1", .user_id = "u-1", .name = "Backyard",
      .latitude = -20.2, .longitude = 57.5,
      .bortle_class = std::optional<int16_t>(5),
    };

    auto updated = Nyx::Domain::ObservingLocation{
      .id = "loc-1", .user_id = "u-1", .name = "Backyard Updated",
      .latitude = -20.3, .longitude = 57.6,
      .bortle_class = std::optional<int16_t>(4),
    };

    auto request = UpdateLocationRequest{
      .name = "Backyard Updated", .latitude = -20.3, .longitude = 57.6,
      .bortle_class = std::optional<int16_t>(4),
    };

    EXPECT_CALL(*this->location_repo, find_by_id("loc-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::ObservingLocation>(existing)
      ));
    EXPECT_CALL(
      *this->location_repo,
      find_by_user_id_and_name("u-1", "Backyard Updated")
    ).WillOnce(::testing::Return(
      std::optional<Nyx::Domain::ObservingLocation>(std::nullopt)
    ));
    EXPECT_CALL(*this->location_repo, update(::testing::_))
      .WillOnce(::testing::Return(updated));

    auto result = this->service->update_location(
      "u-1", "loc-1", request, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "Backyard Updated");
    EXPECT_EQ(result->bortle_class.value(), 4);
  }

  TEST_F(LocationServiceTest, UpdateLocationDuplicateNameConflict) {
    auto existing = Nyx::Domain::ObservingLocation{
      .id = "loc-1", .user_id = "u-1", .name = "Backyard",
      .latitude = -20.2, .longitude = 57.5,
      .bortle_class = std::nullopt,
    };

    auto other_location = Nyx::Domain::ObservingLocation{
      .id = "loc-2", .user_id = "u-1", .name = "Dark Site",
      .latitude = -20.5, .longitude = 57.7,
      .bortle_class = std::nullopt,
    };

    auto request = UpdateLocationRequest{
      .name = "Dark Site", .latitude = -20.2, .longitude = 57.5,
      .bortle_class = std::nullopt,
    };

    EXPECT_CALL(*this->location_repo, find_by_id("loc-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::ObservingLocation>(existing)
      ));
    EXPECT_CALL(
      *this->location_repo,
      find_by_user_id_and_name("u-1", "Dark Site")
    ).WillOnce(::testing::Return(
      std::optional<Nyx::Domain::ObservingLocation>(other_location)
    ));

    auto result = this->service->update_location(
      "u-1", "loc-1", request, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::Conflict);
  }

  TEST_F(LocationServiceTest, UpdateLocationSameNameSkipsDuplicateCheck) {
    auto existing = Nyx::Domain::ObservingLocation{
      .id = "loc-1", .user_id = "u-1", .name = "Backyard",
      .latitude = -20.2, .longitude = 57.5,
      .bortle_class = std::optional<int16_t>(5),
    };

    auto updated = Nyx::Domain::ObservingLocation{
      .id = "loc-1", .user_id = "u-1", .name = "Backyard",
      .latitude = -20.3, .longitude = 57.6,
      .bortle_class = std::optional<int16_t>(4),
    };

    auto request = UpdateLocationRequest{
      .name = "Backyard", .latitude = -20.3, .longitude = 57.6,
      .bortle_class = std::optional<int16_t>(4),
    };

    EXPECT_CALL(*this->location_repo, find_by_id("loc-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::ObservingLocation>(existing)
      ));
    EXPECT_CALL(*this->location_repo, update(::testing::_))
      .WillOnce(::testing::Return(updated));

    auto result = this->service->update_location(
      "u-1", "loc-1", request, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->latitude, -20.3);
  }

  TEST_F(LocationServiceTest, UpdateLocationOwnershipDenied) {
    auto existing = Nyx::Domain::ObservingLocation{
      .id = "loc-1", .user_id = "other-user", .name = "Backyard",
      .latitude = -20.2, .longitude = 57.5,
      .bortle_class = std::nullopt,
    };

    auto request = UpdateLocationRequest{
      .name = "Hacked", .latitude = 0.0, .longitude = 0.0,
      .bortle_class = std::nullopt,
    };

    EXPECT_CALL(*this->location_repo, find_by_id("loc-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::ObservingLocation>(existing)
      ));

    auto result = this->service->update_location(
      "u-1", "loc-1", request, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::PermissionDenied);
  }

  TEST_F(LocationServiceTest, DeleteLocationSuccess) {
    auto existing = Nyx::Domain::ObservingLocation{
      .id = "loc-1", .user_id = "u-1", .name = "Backyard",
      .latitude = -20.2, .longitude = 57.5,
      .bortle_class = std::nullopt,
    };

    EXPECT_CALL(*this->location_repo, find_by_id("loc-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::ObservingLocation>(existing)
      ));
    EXPECT_CALL(*this->location_repo, remove("loc-1"))
      .WillOnce(::testing::Return(Nyx::Core::Result<void>{}));

    auto result = this->service->delete_location(
      "u-1", "loc-1", this->logger
    );

    ASSERT_TRUE(result.has_value());
  }

  TEST_F(LocationServiceTest, DeleteLocationOwnershipDenied) {
    auto existing = Nyx::Domain::ObservingLocation{
      .id = "loc-1", .user_id = "other-user", .name = "Backyard",
      .latitude = -20.2, .longitude = 57.5,
      .bortle_class = std::nullopt,
    };

    EXPECT_CALL(*this->location_repo, find_by_id("loc-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::ObservingLocation>(existing)
      ));

    auto result = this->service->delete_location(
      "u-1", "loc-1", this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::PermissionDenied);
  }
} // namespace Nyx::Application::Location::Tests
