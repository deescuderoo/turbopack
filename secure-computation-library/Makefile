BUILD_DIR=build
DEBUG_BUILD_DIR=${BUILD_DIR}/Debug
RELEASE_BUILD_DIR=${BUILD_DIR}/Release
RELEASE_OUTPUT_DIR=lib
EXAMPLES_DIR=examples
EXAMPLES_BUILD_DIR=${EXAMPLES_DIR}/build

# not the nicest because it has to be maintained separately.
define targets
Available targets:
  dev           build for development
  test          build for development and run tests
  coverage      build for development and compute test coverage
  release       build .so file
  documentation build documentation
  examples      build example applications in examples/
  clean         delete all build files
  clean_all     delete all build files, documentation, .so files and build files
                for the example applications
endef

default:
	$(info ${targets})
	@:

_create_build_dir:
	$(info Creating build directory)
	@mkdir -p ${BUILD_DIR}

# debug targets
_cmake_debug: _create_build_dir
	$(info Creating debug directory)
	@cmake -DCMAKE_BUILD_TYPE=Debug -B ${DEBUG_BUILD_DIR}

dev: _cmake_debug
	@cd ${DEBUG_BUILD_DIR} && make -s -j4

test: dev
	$(info Running tests)
	@cd ${DEBUG_BUILD_DIR} && ./tests.x

coverage: _cmake_debug
	$(info Computing test coverage)
	@cd ${DEBUG_BUILD_DIR} && make -s -j4 coverage

# release targets
_cmake_release: _create_build_dir
	$(info Creating release directory)
	@cmake -DCMAKE_BUILD_TYPE=Release -B ${RELEASE_BUILD_DIR}

release: _cmake_release
	$(info Building release version)
	@cd ${RELEASE_BUILD_DIR} && make
	@mkdir -p ${RELEASE_OUTPUT_DIR}
	@cp "${RELEASE_BUILD_DIR}/libscl.so" ${RELEASE_OUTPUT_DIR}

documentation: _cmake_release
	$(info Building documentation)
	@cd ${RELEASE_BUILD_DIR} && make -s documentation

# examples
examples: release
	@cd ${EXAMPLES_DIR} && cmake . -B build
	@cd ${EXAMPLES_BUILD_DIR} && make -s

# cleanups
clean:
	@rm -rf build

clean_all: clean
	@rm -rf doc
	@rm -rf lib
	@rm -rf ${EXAMPLES_BUILD_DIR}

# other
generate_compile_commands_json: _cmake_debug
	@cd ${DEBUG_BUILD_DIR} && make -s clean && bear -- make -s -j4
	@cp ${DEBUG_BUILD_DIR}/compile_commands.json .

.PHONY: default
	_create_build_dir
	_cmake_debug
	dev
	test
	coverage
	_cmake_release
	release
	clean
	clean_all
	generate_compile_commands_json
