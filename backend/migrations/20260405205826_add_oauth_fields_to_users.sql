-- +goose Up
-- +goose StatementBegin
ALTER TABLE users
  ADD COLUMN auth_provider VARCHAR(20) NOT NULL DEFAULT 'local',
  ADD COLUMN google_id VARCHAR(255),
  ALTER COLUMN password_hash DROP NOT NULL;

CREATE UNIQUE INDEX idx_users_google_id
  ON users(google_id) WHERE google_id IS NOT NULL;
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
DROP INDEX IF EXISTS idx_users_google_id;

ALTER TABLE users
  DROP COLUMN google_id,
  DROP COLUMN auth_provider,
  ALTER COLUMN password_hash SET NOT NULL;
-- +goose StatementEnd
