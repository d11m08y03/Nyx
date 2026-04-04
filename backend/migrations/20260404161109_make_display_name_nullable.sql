-- +goose Up
-- +goose StatementBegin
ALTER TABLE users ALTER COLUMN display_name DROP NOT NULL;
ALTER TABLE users ALTER COLUMN display_name SET DEFAULT NULL;
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
UPDATE users SET display_name = email WHERE display_name IS NULL;
ALTER TABLE users ALTER COLUMN display_name SET NOT NULL;
-- +goose StatementEnd
