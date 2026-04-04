-- +goose Up
-- +goose StatementBegin
CREATE TABLE cameras (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  name VARCHAR(255) NOT NULL,
  sensor_type VARCHAR(50) NOT NULL,
  brand VARCHAR(255),
  model VARCHAR(255),
  pixel_size_um DOUBLE PRECISION,
  resolution VARCHAR(50),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_cameras_user_id ON cameras(user_id);
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
DROP TABLE IF EXISTS cameras;
-- +goose StatementEnd
