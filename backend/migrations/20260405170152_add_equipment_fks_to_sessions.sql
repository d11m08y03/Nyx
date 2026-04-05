-- +goose Up
ALTER TABLE observation_sessions
  ADD COLUMN telescope_id UUID NOT NULL REFERENCES telescopes(id),
  ADD COLUMN camera_id UUID NOT NULL REFERENCES cameras(id),
  ADD COLUMN mount_id UUID NOT NULL REFERENCES mounts(id),
  ADD COLUMN location_id UUID NOT NULL REFERENCES observing_locations(id),
  ADD COLUMN filter_id UUID REFERENCES filters(id);

CREATE INDEX idx_observation_sessions_telescope_id ON observation_sessions(telescope_id);
CREATE INDEX idx_observation_sessions_camera_id ON observation_sessions(camera_id);
CREATE INDEX idx_observation_sessions_mount_id ON observation_sessions(mount_id);
CREATE INDEX idx_observation_sessions_location_id ON observation_sessions(location_id);
CREATE INDEX idx_observation_sessions_filter_id ON observation_sessions(filter_id);

-- +goose Down
DROP INDEX IF EXISTS idx_observation_sessions_filter_id;
DROP INDEX IF EXISTS idx_observation_sessions_location_id;
DROP INDEX IF EXISTS idx_observation_sessions_mount_id;
DROP INDEX IF EXISTS idx_observation_sessions_camera_id;
DROP INDEX IF EXISTS idx_observation_sessions_telescope_id;

ALTER TABLE observation_sessions
  DROP COLUMN IF EXISTS filter_id,
  DROP COLUMN IF EXISTS location_id,
  DROP COLUMN IF EXISTS mount_id,
  DROP COLUMN IF EXISTS camera_id,
  DROP COLUMN IF EXISTS telescope_id;
