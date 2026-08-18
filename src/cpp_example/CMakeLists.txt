# =============================================================================
# YOLO26 example applications
#
# Every <category>/<model>/ directory holding <model>_sync.cpp / <model>_async.cpp
# becomes a target of the same name, so adding another YOLO26 variant is a matter
# of dropping in the sources - no edit here.
# =============================================================================
set(CPP_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Sources shared by every application, compiled once.
add_library(dxyolo26_common OBJECT
    ${PROJECT_ROOT}/src/utility/common_util.cpp
)
target_include_directories(dxyolo26_common PUBLIC
    ${CPP_EXAMPLE_DIR}
    ${PROJECT_ROOT}/src/utility
    ${PROJECT_ROOT}/extern
    ${DXRT_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)
target_compile_definitions(dxyolo26_common PUBLIC PROJECT_ROOT_DIR="${DXYOLO26_DATA_DIR}")

set(DXYOLO26_APP_LIBS
    dxyolo26_common
    ${OpenCV_LIBS}
    ${DXRT_LIBRARY}
    Threads::Threads
    ${DXYOLO26_FS_LIB}
)

if(DXYOLO26_VARIANTS STREQUAL "both")
    set(_VARIANTS sync async)
elseif(DXYOLO26_VARIANTS STREQUAL "sync" OR DXYOLO26_VARIANTS STREQUAL "async")
    set(_VARIANTS ${DXYOLO26_VARIANTS})
else()
    message(FATAL_ERROR "DXYOLO26_VARIANTS must be one of: both, sync, async (got '${DXYOLO26_VARIANTS}')")
endif()

set(DXYOLO26_APPS "")

file(GLOB _CATEGORIES RELATIVE ${CPP_EXAMPLE_DIR} ${CPP_EXAMPLE_DIR}/*)
foreach(CATEGORY ${_CATEGORIES})
    if(NOT IS_DIRECTORY ${CPP_EXAMPLE_DIR}/${CATEGORY} OR CATEGORY STREQUAL "common")
        continue()
    endif()

    file(GLOB _MODELS RELATIVE ${CPP_EXAMPLE_DIR}/${CATEGORY} ${CPP_EXAMPLE_DIR}/${CATEGORY}/*)
    foreach(MODEL ${_MODELS})
        set(_MODEL_DIR ${CPP_EXAMPLE_DIR}/${CATEGORY}/${MODEL})
        if(NOT IS_DIRECTORY ${_MODEL_DIR})
            continue()
        endif()

        foreach(VARIANT ${_VARIANTS})
            set(_SRC ${_MODEL_DIR}/${MODEL}_${VARIANT}.cpp)
            if(NOT EXISTS ${_SRC})
                continue()
            endif()
            add_executable(${MODEL}_${VARIANT} ${_SRC})
            target_link_libraries(${MODEL}_${VARIANT} PRIVATE ${DXYOLO26_APP_LIBS})
            list(APPEND DXYOLO26_APPS ${MODEL}_${VARIANT})
        endforeach()
    endforeach()
endforeach()

if(NOT DXYOLO26_APPS)
    message(FATAL_ERROR "No application sources found under ${CPP_EXAMPLE_DIR}")
endif()

message(STATUS "  applications        : ${DXYOLO26_APPS}")

install(TARGETS ${DXYOLO26_APPS} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
