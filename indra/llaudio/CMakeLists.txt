# -*- cmake -*-

project(llaudio)

include(00-Common)
include(Audio)
include(LLCommon)
include(LLMath)
include(LLMessage)
include(LLVFS)
include(LLMedia)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${OGG_INCLUDE_DIRS}
    ${VORBISENC_INCLUDE_DIRS}
    ${VORBISFILE_INCLUDE_DIRS}
    ${VORBIS_INCLUDE_DIRS}
    ${LLMEDIA_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    )

set(llaudio_SOURCE_FILES
    audioengine.cpp
    listener.cpp
    llaudiodecodemgr.cpp
    vorbisdecode.cpp
    vorbisencode.cpp
	audioengine_openal.cpp
	listener_openal.cpp
    )

set(llaudio_HEADER_FILES
    CMakeLists.txt

    audioengine.h
    listener.h
    llaudiodecodemgr.h
    vorbisdecode.h
    vorbisencode.h
	listener_openal.h
	audioengine_openal.h
    )

set_source_files_properties(${llaudio_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llaudio_SOURCE_FILES ${llaudio_HEADER_FILES})

add_library (llaudio ${llaudio_SOURCE_FILES})

if (DARWIN)
	include(CMakeFindFrameworks)
	find_library(OPENAL_LIBRARY OpenAL)
	mark_as_advanced(QUICKTIME_LIBRARY)
else (DARWIN)
	set(OPENAL_LIBRARY openal)
endif (DARWIN)

target_link_libraries(
    llaudio
    ${VORBISENC_LIBRARIES}
    ${VORBISFILE_LIBRARIES}
    ${VORBIS_LIBRARIES}
    ${OGG_LIBRARIES}
	alut
	${OPENAL_LIBRARY}
    )
