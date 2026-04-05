-- +goose Up
-- +goose StatementBegin
CREATE TABLE tess_observations (
  id UUID PRIMARY KEY,
  target_id UUID NOT NULL REFERENCES targets(id) ON DELETE CASCADE,
  obsid VARCHAR(20) NOT NULL UNIQUE,
  cadence_seconds INTEGER NOT NULL,
  start_time DOUBLE PRECISION NOT NULL,
  end_time DOUBLE PRECISION NOT NULL,
  data_uri VARCHAR(500),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX idx_tess_observations_target_id ON tess_observations(target_id);
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
DROP TABLE IF EXISTS tess_observations;
-- +goose StatementEnd
