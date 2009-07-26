# -*- cmake -*-
include(Linking)
include(Prebuilt)

set(MOZLIB ON CACHE BOOL 
    "Enable Mozilla support in the viewer (requires llmozlib library).")

if (MOZLIB)
    if (STANDALONE)
        set(MOZLIB_FIND_QUIETLY OFF)
        include(FindMozlib)
        if (MOZLIB_SHARED OR MOZLIB_STATIC)
	        MESSAGE ( STATUS "llmozlib2 found, embedded web support enabled" )
	        set(MOZLIB ON)
    		add_definitions(-DLL_LLMOZLIB_ENABLED=1)
        else (MOZLIB_SHARED OR MOZLIB_STATIC)
	        MESSAGE ( STATUS "llmozlib2 NOT found, embedded web support DISABLED")
        endif (MOZLIB_SHARED OR MOZLIB_STATIC)
    else (STANDALONE)
        use_prebuilt_binary(llmozlib)
	add_definitions(-DLL_LLMOZLIB_ENABLED=1)
    endif (STANDALONE)


    if (LINUX)
        if(NOT STANDALONE)
            link_directories(${CMAKE_SOURCE_DIR}/newview/app_settings/mozilla-runtime-linux-${ARCH})
        endif(NOT STANDALONE)
        
        if(MOZLIB_STATIC OR NOT STANDALONE)
            set(MOZLIB_LIBRARIES
                llmozlib2
                mozjs
                nspr4
                plc4
                plds4
                xpcom
                xul
                profdirserviceprovider_s
                )
        elseif(MOZLIB_SHARED)
            set(MOZLIB_LIBRARIES
                llmozlib2
                )
        endif(MOZLIB_STATIC OR NOT STANDALONE)
    elseif (WINDOWS)
        if (MSVC71)
            set(MOZLIB_LIBRARIES 
                debug llmozlib2d
                optimized llmozlib2)
        elseif (MSVC80 OR MSVC90)
            set(MOZLIB_LIBRARIES 
                debug llmozlib2d-vc80
                optimized llmozlib2-vc80)
        endif (MSVC71)
    else (LINUX)
        set(MOZLIB_LIBRARIES
          optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libllmozlib2.dylib
          debug ${ARCH_PREBUILT_DIRS_DEBUG}/libllmozlib2.dylib
          )
    endif (LINUX)
else (MOZLIB)
    add_definitions(-DLL_LLMOZLIB_ENABLED=0)
endif (MOZLIB)
