# -*- cmake -*-

# Find Mozlib
# Try to find the llmozlib libraries when building standalone
# There are a number of different ways this could happen
# 1)    A custom build (or a copy of) the offical LL mozilla based mozlib libray, this results
#       in static mozlib and requires some other libraries to be linked against
#
# 2)    A shared mozlib, as per the Debian Package by Robin Cornelius, this results in a shared
#       library version of llmozlib that has its own correct dependencies and requires no other
#       linkages to the viewer
#
# 3)    A shared mozlib, based on qtwebkit as per the Debian package by Robin Cornlius, this 
#       again has its own dependencies, we may want to add additional checks and enabled additional
#       features here, but it is hoped that 1 and 2 are legacy and this will replace

FIND_PATH(MOZLIB_INCLUDE_DIR llmozlib2.h
    /usr/local/include/
    /usr/include/
)

SET(MOZLIB_SHARED_NAMES ${MOZLIB_SHARED_NAMES} llmozlib2)
SET(MOZLIB_STATIC_NAMES ${MOZLIB_STATIC_NAMES} libllmozlib2.a)
SET(MOZLIB_PROFDIR_NAMES ${MOZLIB_PROFDIR_NAMES} libprofdirserviceprovider_s.a)

FIND_LIBRARY(MOZLIB_SHARED_LIBRARY
    NAMES ${MOZLIB_SHARED_NAMES}
    PATHS /usr/lib /usr/local/lib 
)

FIND_FILE(MOZLIB_STATIC_LIBRARY
    NAMES ${MOZLIB_SHARED_NAMES}
    PATHS /usr/lib /usr/local/lib 
)

FIND_FILE(MOZLIB_PROFDIR_LIBRARY
    NAMES ${MOZLIB_PROFDIR_NAMES}
    PATHS /usr/lib /usr/local/lib 
)

# We have a number of possible outcomes

#1 No Mozlib found at all
#2 Static mozlib found in a user location
#3 Shared mozlib found in a user location
#4 Both shared and static found in a user location

#TODO search for  libprofdirserviceprovider_s.a, if LL decide to ship qtwebkit with
#a shared libary for linux, this files presence will tell us if we are using qtwebkit-mozlib or xulrunner-mozlib

set(MOZLIB_FOUND OFF)

if (MOZLIB_INCLUDE_DIR AND MOZLIB_SHARED_LIBRARY AND NOT MOZLIB_STATIC_LIBRARY)
    set(MOZLIB_SHARED ON)
    set(MOZLIB_FOUND ON)
    if(NOT MOZLIB_FIND_QUIETLY)
        MESSAGE("-- Found a SHARED llmozlib library in ${MOZLIB_SHARED_LIBRARY} and header in ${MOZLIB_INCLUDE_DIR}")
    endif(NOT MOZLIB_FIND_QUIETLY)
endif (MOZLIB_INCLUDE_DIR AND MOZLIB_SHARED_LIBRARY AND NOT MOZLIB_STATIC_LIBRARY)

if (MOZLIB_INCLUDE_DIR AND MOZLIB_STATIC_LIBRARY AND NOT MOZLIB_SHARED_LIBRARY)
    set(MOZLIB_STATIC ON)
    set(MOZLIB_FOUND ON)    
    if(NOT MOZLIB_FIND_QUIETLY)
        MESSAGE("-- Found a STATIC llmozlib library in ${MOZLIB_STATIC_LIBRARY} and header in ${MOZLIB_INCLUDE_DIR}")
    endif(NOT MOZLIB_FIND_QUIETLY)
endif (MOZLIB_INCLUDE_DIR AND MOZLIB_STATIC_LIBRARY AND NOT MOZLIB_SHARED_LIBRARY)

if (MOZLIB_INCLUDE_DIR AND MOZLIB_STATIC_LIBRARY AND MOZLIB_SHARED_LIBRARY)
    set(MOZLIB_FOUND ON)
    set(MOZLIB_SHARED_LIBRARY OFF)
    if(NOT MOZLIB_FIND_QUIETLY)
        MESSAGE("WARNING Both shared and static llmozlib detected, i'm defaulting to the static one in ${MOZLIB_STATIC_LIBRARY} and header in ${MOZLIB_INCLUDE_DIR}")
    endif(NOT MOZLIB_FIND_QUIETLY)
endif (MOZLIB_INCLUDE_DIR AND MOZLIB_STATIC_LIBRARY AND MOZLIB_SHARED_LIBRARY)


