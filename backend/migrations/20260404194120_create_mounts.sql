-- +goose Up
-- +goose StatementBegin
CREATE TABLE mounts (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  name VARCHAR(255) NOT NULL,
  mount_type VARCHAR(50) NOT NULL,
  is_tracked BOOLEAN NOT NULL DEFAULT FALSE,
  has_goto BOOLEAN NOT NULL DEFAULT FALSE,
  brand VARCHAR(255),
  model VARCHAR(255),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_mounts_user_id ON mounts(user_id);
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
DROP TABLE IF EXISTS mounts;
-- +goose StatementEnd
