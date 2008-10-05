# -*- cmake -*-

include(Prebuilt)

use_prebuilt_binary(havok)

set(HAVOK_VERSION 460)

set(LLPHYSICS_INCLUDE_DIRS
    ${LIBS_SERVER_DIR}/llphysics
    ${LIBS_PREBUILT_DIR}/include/havok/hk${HAVOK_VERSION}/common
    ${LIBS_PREBUILT_DIR}/include/havok/hk${HAVOK_VERSION}/physics
    )

add_definitions(-DLL_CURRENT_HAVOK_VERSION=${HAVOK_VERSION})

set(LLPHYSICS_LIBRARIES llphysics)
