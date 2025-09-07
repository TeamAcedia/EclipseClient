# Always run during 'make'

if(NOT VERSION_GITHASH)
	set(VERSION_GITHASH "${VERSION_STRING}")
endif()

configure_file(
	${GENERATE_VERSION_SOURCE_DIR}/cmake_config_githash.h.in
	${GENERATE_VERSION_BINARY_DIR}/cmake_config_githash.h)

