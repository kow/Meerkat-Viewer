# -*- cmake -*-
include(Prebuilt)


if (STANDALONE)
  include(FindPkgConfig)

  pkg_check_modules(GSTREAMER REQUIRED gstreamer-0.10)
  pkg_check_modules(GSTREAMER_PLUGINS_BASE REQUIRED gstreamer-plugins-base-0.10)
else (STANDALONE)
if (NOT DARWIN)
  use_prebuilt_binary(gstreamer)
  # possible libxml should have its own .cmake file instead
  use_prebuilt_binary(libxml)
  set(GSTREAMER_FOUND ON FORCE BOOL)
  set(GSTREAMER_PLUGINS_BASE_FOUND ON FORCE BOOL)
  set(GSTREAMER_INCLUDE_DIRS
      ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include/gstreamer-0.10
      ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include/glib-2.0
      ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include/libxml2
      )
  # We don't need to explicitly link against gstreamer itself, because
  # LLMediaImplGStreamer probes for the system's copy at runtime.
  if(LINUX)
    set(GSTREAMER_LIBRARIES
      gobject-2.0
      gmodule-2.0
      gthread-2.0
	  dl
      rt
      glib-2.0
      )
elseif(WINDOWS)
	set(GSTREAMER_LIBRARIES
      gobject-2.0
      gmodule-2.0
      gthread-2.0
	  gstreamer-0.10
	  gstreamer-plugins-base-0.10
      glib-2.0
      )
endif(LINUX)
endif (NOT DARWIN)
endif (STANDALONE)

if (GSTREAMER_FOUND AND GSTREAMER_PLUGINS_BASE_FOUND)
  set(GSTREAMER ON CACHE BOOL "Build with GStreamer streaming media support.")
endif (GSTREAMER_FOUND AND GSTREAMER_PLUGINS_BASE_FOUND)

if (GSTREAMER)
  add_definitions(-DLL_GSTREAMER_ENABLED=1)
endif (GSTREAMER)
