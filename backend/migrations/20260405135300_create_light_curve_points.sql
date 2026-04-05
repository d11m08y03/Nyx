-- +goose Up
CREATE TABLE light_curve_points (
  id BIGSERIAL PRIMARY KEY,
  tess_observation_id UUID NOT NULL REFERENCES tess_observations(id) ON DELETE CASCADE,
  time DOUBLE PRECISION NOT NULL,
  pdcsap_flux REAL,
  pdcsap_flux_err REAL,
  sap_flux REAL,
  quality INTEGER NOT NULL DEFAULT 0
);

CREATE INDEX idx_lcp_observation_id ON light_curve_points(tess_observation_id);
CREATE INDEX idx_lcp_observation_time ON light_curve_points(tess_observation_id, time);

-- +goose Down
DROP TABLE IF EXISTS light_curve_points;
