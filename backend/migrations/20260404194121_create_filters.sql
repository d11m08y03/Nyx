-- +goose Up
-- +goose StatementBegin
CREATE TABLE filters (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  name VARCHAR(255) NOT NULL,
  band VARCHAR(50) NOT NULL,
  bandwidth_nm DOUBLE PRECISION,
  brand VARCHAR(255),
  model VARCHAR(255),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_filters_user_id ON filters(user_id);
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
DROP TABLE IF EXISTS filters;
-- +goose StatementEnd
