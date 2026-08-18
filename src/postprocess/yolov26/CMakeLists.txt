# =============================================================================
# libdxapp_yolov26_postprocess - YOLO26 postprocessing as a shared library
#
# The output name is kept identical to the one dx_app produces so that existing
# consumers (dx_stream pipelines, python bindings) keep working unchanged.
# =============================================================================
set(SOURCES
    ${PROJECT_ROOT}/src/utility/common_util.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/yolov26_postprocess.cpp
)

set(HEADERS
    ${PROJECT_ROOT}/src/utility/common_util.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/yolov26_postprocess.h
)

add_library(yolov26_postprocess SHARED ${SOURCES})

set_target_properties(yolov26_postprocess PROPERTIES
    OUTPUT_NAME "dxapp_yolov26_postprocess"
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(yolov26_postprocess PRIVATE
    ${PROJECT_ROOT}/src/utility
    ${PROJECT_ROOT}/src/cpp_example
    ${DXRT_INCLUDE_DIR}
)

target_compile_definitions(yolov26_postprocess PRIVATE PROJECT_ROOT_DIR="${DXYOLO26_DATA_DIR}")
target_link_libraries(yolov26_postprocess PRIVATE ${DXRT_LIBRARY} ${DXYOLO26_FS_LIB})

install(TARGETS yolov26_postprocess LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dx_yolo26)
