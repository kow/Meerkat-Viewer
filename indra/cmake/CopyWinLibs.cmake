# -*- cmake -*-

# The copy_win_libs folder contains file lists and a script used to 
# copy dlls, exes and such needed to run the SecondLife from within 
# VisualStudio. 

include(CMakeCopyIfDifferent)

set(vivox_src_dir "${CMAKE_SOURCE_DIR}/newview/vivox-runtime/i686-win32")
set(vivox_files
    tntk.dll
    libeay32.dll
    ssleay32.dll
    srtp.dll
    ortp.dll
    wrap_oal.dll
    )

set(debug_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-win32/lib/debug")
set(debug_files
    dbghelp.dll 
    nspr4.dll 
    gksvggdiplus.dll 
    zlib1.dll 
    ssl3.dll 
    nss3.dll 
    libgstbase-0.10.dll 
    nssckbi.dll 
    smime3.dll 
    softokn3.dll 
    xul.dll 
    libgstaudio-0.10.dll 
    libglib-2.0-0.dll 
    intl.dll 
    libxml2.dll 
    libgmodule-2.0-0.dll 
    js3250.dll 
    libgstvideo-0.10.dll 
    libgobject-2.0-0.dll 
    libgio-2.0-0.dll 
    msvcp71.dll 
    msvcr71.dll 
    libgstplaybin.dll 
    alut.dll 
    OpenJPEGd.dll 
    freebl3.dll 
    plds4.dll 
    xpcom.dll 
    libgstreamer-0.10.dll
    libgthread-2.0-0.dll 
    plc4.dll 
    libgstpbutils-0.10.dll
    libgstinterfaces-0.10.dll
    iconv.dll 
    OpenAL32.dll 
    )

copy_if_different(
    ${debug_src_dir} 
    "${CMAKE_CURRENT_BINARY_DIR}/Debug"
    out_targets 
    ${debug_files}
    )
set(all_targets ${all_targets} ${out_targets})

copy_if_different(
    ${vivox_src_dir} 
    "${CMAKE_CURRENT_BINARY_DIR}/Debug"
    out_targets 
    ${vivox_files}
    )
set(all_targets ${all_targets} ${out_targets})

set(release_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-win32/lib/release")
set(release_files
    dbghelp.dll 
    nspr4.dll 
    OpenJPEG.dll 
    gksvggdiplus.dll 
    zlib1.dll 
    ssl3.dll 
    nss3.dll 
    libgstbase-0.10.dll 
    nssckbi.dll 
    smime3.dll 
    softokn3.dll 
    xul.dll 
    libgstaudio-0.10.dll 
    libglib-2.0-0.dll 
    intl.dll 
    libxml2.dll 
    libgmodule-2.0-0.dll 
    js3250.dll 
    libgstvideo-0.10.dll 
    libgobject-2.0-0.dll 
    libgio-2.0-0.dll 
    msvcp71.dll 
    msvcr71.dll 
    libgstplaybin.dll 
    alut.dll 
    iconv.dll 
    freebl3.dll 
    plds4.dll 
    xpcom.dll 
    libgstreamer-0.10.dll
    libgthread-2.0-0.dll 
    plc4.dll 
    libgstpbutils-0.10.dll
    libgstinterfaces-0.10.dll
    OpenAL32.dll 
    )
    
copy_if_different(
    ${release_src_dir} 
    "${CMAKE_CURRENT_BINARY_DIR}/Release"
    out_targets 
    ${release_files}
    )
set(all_targets ${all_targets} ${out_targets})

copy_if_different(
    ${vivox_src_dir} 
    "${CMAKE_CURRENT_BINARY_DIR}/Release"
    out_targets 
    ${vivox_files}
    )
set(all_targets ${all_targets} ${out_targets})

copy_if_different(
    ${release_src_dir} 
    "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo"
    out_targets 
    ${release_files}
    )
set(all_targets ${all_targets} ${out_targets})

copy_if_different(
    ${vivox_src_dir} 
    "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo"
    out_targets 
    ${vivox_files}
    )
set(all_targets ${all_targets} ${out_targets})

add_custom_target(copy_win_libs ALL DEPENDS ${all_targets})
