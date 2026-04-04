#include "application/equipment/EquipmentService.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace Nyx::Application::Equipment::Tests {
  class MockTelescopeRepository : public Nyx::Domain::ITelescopeRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::Telescope>, create,
        (const Nyx::Domain::Telescope& telescope), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::Telescope>>),
        find_by_user_id, (const std::string& user_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::Telescope>>),
        find_by_id, (const std::string& id), (override)
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

  class MockCameraRepository : public Nyx::Domain::ICameraRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::Camera>, create,
        (const Nyx::Domain::Camera& camera), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::Camera>>),
        find_by_user_id, (const std::string& user_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::Camera>>),
        find_by_id, (const std::string& id), (override)
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

  class MockMountRepository : public Nyx::Domain::IMountRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::Mount>, create,
        (const Nyx::Domain::Mount& mount), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::Mount>>),
        find_by_user_id, (const std::string& user_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::Mount>>),
        find_by_id, (const std::string& id), (override)
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

  class MockFilterRepository : public Nyx::Domain::IFilterRepository {
    public:
      MOCK_METHOD(
        Nyx::Core::Result<Nyx::Domain::Filter>, create,
        (const Nyx::Domain::Filter& filter), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::vector<Nyx::Domain::Filter>>),
        find_by_user_id, (const std::string& user_id), (override)
      );
      MOCK_METHOD(
        (Nyx::Core::Result<std::optional<Nyx::Domain::Filter>>),
        find_by_id, (const std::string& id), (override)
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

  class MockUuidGenerator : public Nyx::Core::IUuidGenerator {
    public:
      MOCK_METHOD(std::string, generate, (), (override));
  };

  class EquipmentServiceTest : public ::testing::Test {
    protected:
      auto SetUp() -> void override {
        this->telescope_repo = std::make_shared<MockTelescopeRepository>();
        this->camera_repo = std::make_shared<MockCameraRepository>();
        this->mount_repo = std::make_shared<MockMountRepository>();
        this->filter_repo = std::make_shared<MockFilterRepository>();
        this->uuid_gen = std::make_shared<MockUuidGenerator>();
        this->logger = spdlog::default_logger();

        this->service = std::make_unique<EquipmentService>(
          this->telescope_repo, this->camera_repo,
          this->mount_repo, this->filter_repo, this->uuid_gen
        );
      }

      std::shared_ptr<MockTelescopeRepository> telescope_repo;
      std::shared_ptr<MockCameraRepository> camera_repo;
      std::shared_ptr<MockMountRepository> mount_repo;
      std::shared_ptr<MockFilterRepository> filter_repo;
      std::shared_ptr<MockUuidGenerator> uuid_gen;
      std::shared_ptr<spdlog::logger> logger;
      std::unique_ptr<EquipmentService> service;
  };

  // --- Telescope CRUD ---

  TEST_F(EquipmentServiceTest, CreateTelescopeSuccess) {
    auto request = CreateTelescopeRequest{
      .name = "ED80", .aperture_mm = 80, .focal_length_mm = 600,
      .optical_design = "refractor", .brand = "Skywatcher", .model = "ED80",
    };

    auto created = Nyx::Domain::Telescope{
      .id = "t-1", .user_id = "u-1", .name = "ED80",
      .aperture_mm = 80, .focal_length_mm = 600,
      .optical_design = "refractor", .brand = "Skywatcher", .model = "ED80",
    };

    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("t-1"));
    EXPECT_CALL(*this->telescope_repo, create(::testing::_))
      .WillOnce(::testing::Return(created));

    auto result = this->service->create_telescope("u-1", request, this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "t-1");
    EXPECT_EQ(result->name, "ED80");
    EXPECT_EQ(result->aperture_mm, 80);
  }

  TEST_F(EquipmentServiceTest, CreateTelescopeDatabaseError) {
    auto request = CreateTelescopeRequest{
      .name = "ED80", .aperture_mm = 80, .focal_length_mm = 600,
      .optical_design = "refractor", .brand = "", .model = "",
    };

    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("t-1"));
    EXPECT_CALL(*this->telescope_repo, create(::testing::_))
      .WillOnce(::testing::Return(
        std::unexpected(Nyx::Core::AppError::internal("DB error"))
      ));

    auto result = this->service->create_telescope("u-1", request, this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::InternalError);
  }

  TEST_F(EquipmentServiceTest, ListTelescopesSuccess) {
    auto telescopes = std::vector<Nyx::Domain::Telescope>{
      {.id = "t-1", .user_id = "u-1", .name = "ED80",
       .aperture_mm = 80, .focal_length_mm = 600,
       .optical_design = "refractor", .brand = "", .model = ""},
      {.id = "t-2", .user_id = "u-1", .name = "SCT8",
       .aperture_mm = 203, .focal_length_mm = 2032,
       .optical_design = "sct", .brand = "Celestron", .model = "C8"},
    };

    EXPECT_CALL(*this->telescope_repo, find_by_user_id("u-1"))
      .WillOnce(::testing::Return(telescopes));

    auto result = this->service->list_telescopes("u-1", this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 2);
    EXPECT_EQ(result->at(0).name, "ED80");
    EXPECT_EQ(result->at(1).name, "SCT8");
  }

  TEST_F(EquipmentServiceTest, ListTelescopesEmpty) {
    EXPECT_CALL(*this->telescope_repo, find_by_user_id("u-1"))
      .WillOnce(::testing::Return(std::vector<Nyx::Domain::Telescope>{}));

    auto result = this->service->list_telescopes("u-1", this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
  }

  TEST_F(EquipmentServiceTest, GetTelescopeSuccess) {
    auto telescope = Nyx::Domain::Telescope{
      .id = "t-1", .user_id = "u-1", .name = "ED80",
      .aperture_mm = 80, .focal_length_mm = 600,
      .optical_design = "refractor", .brand = "", .model = "",
    };

    EXPECT_CALL(*this->telescope_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Telescope>(telescope)
      ));

    auto result = this->service->get_telescope("u-1", "t-1", this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "t-1");
  }

  TEST_F(EquipmentServiceTest, GetTelescopeNotFound) {
    EXPECT_CALL(*this->telescope_repo, find_by_id("bad"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Telescope>(std::nullopt)
      ));

    auto result = this->service->get_telescope("u-1", "bad", this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::ResourceNotFound);
  }

  TEST_F(EquipmentServiceTest, GetTelescopeOwnershipDenied) {
    auto telescope = Nyx::Domain::Telescope{
      .id = "t-1", .user_id = "other-user", .name = "ED80",
      .aperture_mm = 80, .focal_length_mm = 600,
      .optical_design = "refractor", .brand = "", .model = "",
    };

    EXPECT_CALL(*this->telescope_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Telescope>(telescope)
      ));

    auto result = this->service->get_telescope("u-1", "t-1", this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::PermissionDenied);
  }

  TEST_F(EquipmentServiceTest, UpdateTelescopeSuccess) {
    auto existing = Nyx::Domain::Telescope{
      .id = "t-1", .user_id = "u-1", .name = "ED80",
      .aperture_mm = 80, .focal_length_mm = 600,
      .optical_design = "refractor", .brand = "", .model = "",
    };

    auto updated = Nyx::Domain::Telescope{
      .id = "t-1", .user_id = "u-1", .name = "ED80 Pro",
      .aperture_mm = 80, .focal_length_mm = 600,
      .optical_design = "refractor", .brand = "Skywatcher", .model = "ED80",
    };

    auto request = UpdateTelescopeRequest{
      .name = "ED80 Pro", .aperture_mm = 80, .focal_length_mm = 600,
      .optical_design = "refractor", .brand = "Skywatcher", .model = "ED80",
    };

    EXPECT_CALL(*this->telescope_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Telescope>(existing)
      ));
    EXPECT_CALL(*this->telescope_repo, update(::testing::_))
      .WillOnce(::testing::Return(updated));

    auto result = this->service->update_telescope(
      "u-1", "t-1", request, this->logger
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "ED80 Pro");
    EXPECT_EQ(result->brand, "Skywatcher");
  }

  TEST_F(EquipmentServiceTest, UpdateTelescopeOwnershipDenied) {
    auto existing = Nyx::Domain::Telescope{
      .id = "t-1", .user_id = "other-user", .name = "ED80",
      .aperture_mm = 80, .focal_length_mm = 600,
      .optical_design = "refractor", .brand = "", .model = "",
    };

    auto request = UpdateTelescopeRequest{
      .name = "Hacked", .aperture_mm = 80, .focal_length_mm = 600,
      .optical_design = "refractor", .brand = "", .model = "",
    };

    EXPECT_CALL(*this->telescope_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Telescope>(existing)
      ));

    auto result = this->service->update_telescope(
      "u-1", "t-1", request, this->logger
    );

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::PermissionDenied);
  }

  TEST_F(EquipmentServiceTest, DeleteTelescopeSuccess) {
    auto existing = Nyx::Domain::Telescope{
      .id = "t-1", .user_id = "u-1", .name = "ED80",
      .aperture_mm = 80, .focal_length_mm = 600,
      .optical_design = "refractor", .brand = "", .model = "",
    };

    EXPECT_CALL(*this->telescope_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Telescope>(existing)
      ));
    EXPECT_CALL(*this->telescope_repo, remove("t-1"))
      .WillOnce(::testing::Return(Nyx::Core::Result<void>{}));

    auto result = this->service->delete_telescope("u-1", "t-1", this->logger);

    ASSERT_TRUE(result.has_value());
  }

  TEST_F(EquipmentServiceTest, DeleteTelescopeOwnershipDenied) {
    auto existing = Nyx::Domain::Telescope{
      .id = "t-1", .user_id = "other-user", .name = "ED80",
      .aperture_mm = 80, .focal_length_mm = 600,
      .optical_design = "refractor", .brand = "", .model = "",
    };

    EXPECT_CALL(*this->telescope_repo, find_by_id("t-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Telescope>(existing)
      ));

    auto result = this->service->delete_telescope("u-1", "t-1", this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::PermissionDenied);
  }

  // --- Camera CRUD (representative) ---

  TEST_F(EquipmentServiceTest, CreateCameraSuccess) {
    auto request = CreateCameraRequest{
      .name = "ASI294MC", .sensor_type = "CMOS",
      .brand = "ZWO", .model = "ASI294MC Pro",
      .pixel_size_um = 4.63, .resolution = "4144x2822",
    };

    auto created = Nyx::Domain::Camera{
      .id = "c-1", .user_id = "u-1", .name = "ASI294MC",
      .sensor_type = "CMOS", .brand = "ZWO", .model = "ASI294MC Pro",
      .pixel_size_um = 4.63, .resolution = "4144x2822",
    };

    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("c-1"));
    EXPECT_CALL(*this->camera_repo, create(::testing::_))
      .WillOnce(::testing::Return(created));

    auto result = this->service->create_camera("u-1", request, this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "c-1");
    EXPECT_EQ(result->sensor_type, "CMOS");
    EXPECT_DOUBLE_EQ(result->pixel_size_um, 4.63);
  }

  TEST_F(EquipmentServiceTest, GetCameraOwnershipDenied) {
    auto camera = Nyx::Domain::Camera{
      .id = "c-1", .user_id = "other-user", .name = "ASI294MC",
      .sensor_type = "CMOS", .brand = "", .model = "",
      .pixel_size_um = 0.0, .resolution = "",
    };

    EXPECT_CALL(*this->camera_repo, find_by_id("c-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Camera>(camera)
      ));

    auto result = this->service->get_camera("u-1", "c-1", this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::PermissionDenied);
  }

  // --- Mount CRUD (representative) ---

  TEST_F(EquipmentServiceTest, CreateMountSuccess) {
    auto request = CreateMountRequest{
      .name = "EQ6-R Pro", .mount_type = "equatorial",
      .is_tracked = true, .has_goto = true,
      .brand = "Skywatcher", .model = "EQ6-R Pro",
    };

    auto created = Nyx::Domain::Mount{
      .id = "m-1", .user_id = "u-1", .name = "EQ6-R Pro",
      .mount_type = "equatorial", .is_tracked = true, .has_goto = true,
      .brand = "Skywatcher", .model = "EQ6-R Pro",
    };

    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("m-1"));
    EXPECT_CALL(*this->mount_repo, create(::testing::_))
      .WillOnce(::testing::Return(created));

    auto result = this->service->create_mount("u-1", request, this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "m-1");
    EXPECT_TRUE(result->is_tracked);
    EXPECT_TRUE(result->has_goto);
  }

  TEST_F(EquipmentServiceTest, GetMountOwnershipDenied) {
    auto mount = Nyx::Domain::Mount{
      .id = "m-1", .user_id = "other-user", .name = "AZ-GTi",
      .mount_type = "alt-az", .is_tracked = false, .has_goto = true,
      .brand = "", .model = "",
    };

    EXPECT_CALL(*this->mount_repo, find_by_id("m-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Mount>(mount)
      ));

    auto result = this->service->get_mount("u-1", "m-1", this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::PermissionDenied);
  }

  // --- Filter CRUD (representative) ---

  TEST_F(EquipmentServiceTest, CreateFilterSuccess) {
    auto request = CreateFilterRequest{
      .name = "Ha 7nm", .band = "H-alpha",
      .bandwidth_nm = 7.0, .brand = "Optolong", .model = "L-eNhance",
    };

    auto created = Nyx::Domain::Filter{
      .id = "f-1", .user_id = "u-1", .name = "Ha 7nm",
      .band = "H-alpha", .bandwidth_nm = 7.0,
      .brand = "Optolong", .model = "L-eNhance",
    };

    EXPECT_CALL(*this->uuid_gen, generate())
      .WillOnce(::testing::Return("f-1"));
    EXPECT_CALL(*this->filter_repo, create(::testing::_))
      .WillOnce(::testing::Return(created));

    auto result = this->service->create_filter("u-1", request, this->logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "f-1");
    EXPECT_EQ(result->band, "H-alpha");
    EXPECT_DOUBLE_EQ(result->bandwidth_nm, 7.0);
  }

  TEST_F(EquipmentServiceTest, GetFilterOwnershipDenied) {
    auto filter = Nyx::Domain::Filter{
      .id = "f-1", .user_id = "other-user", .name = "L-Pro",
      .band = "broadband", .bandwidth_nm = 0.0,
      .brand = "", .model = "",
    };

    EXPECT_CALL(*this->filter_repo, find_by_id("f-1"))
      .WillOnce(::testing::Return(
        std::optional<Nyx::Domain::Filter>(filter)
      ));

    auto result = this->service->get_filter("u-1", "f-1", this->logger);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, Nyx::Core::ErrorCode::PermissionDenied);
  }
} // namespace Nyx::Application::Equipment::Tests
