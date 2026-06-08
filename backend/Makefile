# Load .env automatically
ifneq (,$(wildcard .env))
	include .env
	export
endif

# Require env vars
.PHONY: check-env
check-env:
	@echo "Checking environment..."
	@test -n "$$DB_URL" || (echo "DB_URL is not set"; exit 1)
	@test -n "$$MIGRATIONS_DIR" || (echo "MIGRATIONS_DIR is not set"; exit 1)

# Migrations (Goose)
.PHONY: migrate-create
migrate-create: check-env
	@read -p "Migration name: " name; \
	goose -dir $$MIGRATIONS_DIR create $$name sql

.PHONY: migrate-up
migrate-up: check-env
	goose -dir $$MIGRATIONS_DIR postgres "$$DB_URL" up

.PHONY: migrate-up-prod
migrate-up-prod: check-env
	goose -dir $$MIGRATIONS_DIR postgres "$$DB_PROD_URL" up

.PHONY: migrate-down
migrate-down: check-env
	goose -dir $$MIGRATIONS_DIR postgres "$$DB_URL" down

.PHONY: migrate-reset
migrate-reset: check-env
	goose -dir $$MIGRATIONS_DIR postgres "$$DB_URL" reset

.PHONY: migrate-status
migrate-status: check-env
	goose -dir $$MIGRATIONS_DIR postgres "$$DB_URL" status

.PHONY: migrate-status-prod
migrate-status-prod: check-env
	goose -dir $$MIGRATIONS_DIR postgres "$$DB_PROD_URL" status

.PHONY: migrate-validate
migrate-validate: check-env
	goose -dir $$MIGRATIONS_DIR postgres "$$DB_URL" validate

# Database tools
.PHONY: db-dump
db-dump: check-env
	@echo "Dumping schema to schema.sql..."
	@pg_dump --schema-only --no-owner --no-privileges "$$DB_URL" > schema.sql
