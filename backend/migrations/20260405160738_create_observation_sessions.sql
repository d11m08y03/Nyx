-- +goose Up
-- +goose StatementBegin
CREATE TABLE observation_sessions (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  target_id UUID NOT NULL REFERENCES targets(id),
  notes TEXT,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_observation_sessions_user_id
  ON observation_sessions(user_id);
CREATE INDEX idx_observation_sessions_target_id
  ON observation_sessions(target_id);

CREATE TABLE observation_images (
  id UUID PRIMARY KEY,
  session_id UUID NOT NULL
    REFERENCES observation_sessions(id) ON DELETE CASCADE,
  filename VARCHAR(255) NOT NULL,
  original_filename VARCHAR(255) NOT NULL,
  file_path VARCHAR(1024) NOT NULL,
  file_size_bytes BIGINT NOT NULL,
  mime_type VARCHAR(50) NOT NULL,
  captured_at TIMESTAMPTZ,
  camera_model VARCHAR(255),
  exposure_time_seconds DOUBLE PRECISION,
  iso_speed INTEGER,
  gps_latitude DOUBLE PRECISION,
  gps_longitude DOUBLE PRECISION,
  image_width INTEGER,
  image_height INTEGER,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_observation_images_session_id
  ON observation_images(session_id);
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
DROP TABLE IF EXISTS observation_images;
DROP TABLE IF EXISTS observation_sessions;
-- +goose StatementEnd
