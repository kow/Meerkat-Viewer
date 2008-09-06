# -*- cmake -*-

include(Audio)

set(LLAUDIO_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llaudio
    )

if (DARWIN)
	include(CMakeFindFrameworks)
	find_library(OPENAL_LIBRARY OpenAL)
	mark_as_advanced(QUICKTIME_LIBRARY)
	set(LLAUDIO_LIBRARIES
	    llaudio
	    ${VORBISENC_LIBRARIES}
	    ${VORBISFILE_LIBRARIES}
	    ${VORBIS_LIBRARIES}
	    ${OGG_LIBRARIES}
		alut
		${OPENAL_LIBRARY}
	    )
else (DARWIN)
	set(LLAUDIO_LIBRARIES
	    llaudio
	    ${VORBISENC_LIBRARIES}
	    ${VORBISFILE_LIBRARIES}
	    ${VORBIS_LIBRARIES}
	    ${OGG_LIBRARIES}
		alut
		openal
	    )
endif (DARWIN)