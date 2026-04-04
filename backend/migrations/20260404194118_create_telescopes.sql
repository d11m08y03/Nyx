-- +goose Up
-- +goose StatementBegin
CREATE TABLE telescopes (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  name VARCHAR(255) NOT NULL,
  aperture_mm INTEGER NOT NULL,
  focal_length_mm INTEGER NOT NULL,
  optical_design VARCHAR(50) NOT NULL,
  brand VARCHAR(255),
  model VARCHAR(255),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_telescopes_user_id ON telescopes(user_id);
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
DROP TABLE IF EXISTS telescopes;
-- +goose StatementEnd
