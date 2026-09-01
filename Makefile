PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

EXT_NAME=biosignals
EXT_CONFIG=${PROJ_DIR}extension_config.cmake

include extension-ci-tools/makefiles/duckdb_extension.Makefile

GARAGE_ENDPOINT = http://localhost:3900
GARAGE_ADMIN_ENDPOINT = http://localhost:3903
GARAGE_ACCESS_KEY = GARAGEADMINACCESSKEY
GARAGE_SECRET_KEY = garagetestingsecretkey
GARAGE_BUCKET = biosignals-test
GARAGE_PROFILE = garage-testing
LOCAL_DATA_SOURCE = ./test/test_data
GARAGE_DATA_DEST = test_data

test_cpp_debug:
	ctest --test-dir build/debug/extension/biosignals/test/cpp

test_cpp_release:
	ctest --test-dir build/release/extension/biosignals/test/cpp

test_cpp_reldebug:
	ctest --test-dir build/reldebug/extension/biosignals/test/cpp

test_debug: test_cpp_debug
	$(MAKE) -f extension-ci-tools/makefiles/duckdb_extension.Makefile test_debug

test_release: test_cpp_release
	$(MAKE) -f extension-ci-tools/makefiles/duckdb_extension.Makefile test_release

test_reldebug: test_cpp_reldebug
	$(MAKE) -f extension-ci-tools/makefiles/duckdb_extension.Makefile test_reldebug

configure_aws_profile:
	aws configure set aws_access_key_id $(GARAGE_ACCESS_KEY) --profile $(GARAGE_PROFILE)
	aws configure set aws_secret_access_key $(GARAGE_SECRET_KEY) --profile $(GARAGE_PROFILE)

setup_garage: configure_aws_profile stop_garage
	podman run -d \
		--name garage \
		-p 3900:3900 -p 3903:3903 \
		-v $(shell pwd)/garage.toml:/etc/garage.toml \
		-e GARAGE_DEFAULT_ACCESS_KEY=$(GARAGE_ACCESS_KEY) \
		-e GARAGE_DEFAULT_SECRET_KEY=$(GARAGE_SECRET_KEY) \
		-e GARAGE_DEFAULT_BUCKET=$(GARAGE_BUCKET) \
		dxflrs/garage:v2.3.0 \
		/garage server --single-node --default-bucket

	@echo "Waiting for Garage to start..."
	@until curl -s $(GARAGE_ADMIN_ENDPOINT)/health; do sleep 1; done

	aws --endpoint-url $(GARAGE_ENDPOINT) --profile $(GARAGE_PROFILE) \
		s3 sync $(LOCAL_DATA_SOURCE) s3://$(GARAGE_BUCKET)/$(GARAGE_DATA_DEST)

stop_garage:
	podman rm -f -v garage
	podman volume rm -f garage.toml
