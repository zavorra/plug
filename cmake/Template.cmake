
set(TEMPLATE_DIR "${CMAKE_MODULE_PATH}/template")
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")


configure_file(${TEMPLATE_DIR}/version.cpp.in
                ${GENERATED_DIR}/version.cpp
                @ONLY
                )

add_library(plug-version ${GENERATED_DIR}/version.cpp)

