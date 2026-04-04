-- +goose Up
-- +goose StatementBegin
CREATE UNIQUE INDEX idx_observing_locations_user_id_name
  ON observing_locations(user_id, name);
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
DROP INDEX IF EXISTS idx_observing_locations_user_id_name;
-- +goose StatementEnd
