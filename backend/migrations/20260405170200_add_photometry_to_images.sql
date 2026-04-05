-- +goose Up
ALTER TABLE observation_images
  ADD COLUMN target_x INTEGER,
  ADD COLUMN target_y INTEGER,
  ADD COLUMN raw_flux DOUBLE PRECISION,
  ADD COLUMN raw_flux_error DOUBLE PRECISION,
  ADD COLUMN relative_flux DOUBLE PRECISION,
  ADD COLUMN relative_flux_error DOUBLE PRECISION,
  ADD COLUMN photometry_status VARCHAR(20),
  ADD COLUMN photometry_error_message TEXT;

-- +goose Down
ALTER TABLE observation_images
  DROP COLUMN IF EXISTS photometry_error_message,
  DROP COLUMN IF EXISTS photometry_status,
  DROP COLUMN IF EXISTS relative_flux_error,
  DROP COLUMN IF EXISTS relative_flux,
  DROP COLUMN IF EXISTS raw_flux_error,
  DROP COLUMN IF EXISTS raw_flux,
  DROP COLUMN IF EXISTS target_y,
  DROP COLUMN IF EXISTS target_x;
