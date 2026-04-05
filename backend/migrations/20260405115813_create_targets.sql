-- +goose Up
-- +goose StatementBegin
CREATE TABLE targets (
  id UUID PRIMARY KEY,
  canonical_name VARCHAR(255) NOT NULL UNIQUE,
  target_type VARCHAR(50),
  right_ascension DOUBLE PRECISION,
  declination DOUBLE PRECISION,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
DROP TABLE IF EXISTS targets;
-- +goose StatementEnd
