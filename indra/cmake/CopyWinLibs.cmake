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
    nssckbi.dll 
    smime3.dll 
    softokn3.dll 
    xul.dll 
    intl.dll 
    libxml2.dll 
    libgmodule-2.0-0.dll 
    js3250.dll 
    libgobject-2.0-0.dll 
    libgio-2.0-0.dll 
    msvcp71.dll 
    msvcr71.dll 
    alut.dll 
    OpenJPEGd.dll 
    freebl3.dll 
    plds4.dll 
    xpcom.dll 
    libgthread-2.0-0.dll 
    plc4.dll 
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
    nssckbi.dll 
    smime3.dll 
    softokn3.dll 
    xul.dll 
    libglib-2.0-0.dll 
    intl.dll 
    libxml2.dll 
    libgmodule-2.0-0.dll 
    js3250.dll 
    libgobject-2.0-0.dll 
    libgio-2.0-0.dll 
    msvcp71.dll 
    msvcr71.dll 
    alut.dll 
    iconv.dll 
    freebl3.dll 
    plds4.dll 
    xpcom.dll 
    libgthread-2.0-0.dll 
    plc4.dll 
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

set(gstreamer_base_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-win32/lib/release/gstreamer/")
set(gstreamer_base_files
    glew32.dll
    liba52-0.dll
    libbz2.dll
    libcairo-2.dll
    libdca-0.dll
    libdl.dll
    libdvdnav-4.dll
    libdvdnavmini-4.dll
    libdvdread-4.dll
    libfaac-0.dll
    libfaad-2.dll
    libfontconfig-1.dll
    libfreetype-6.dll
    libgcrypt-11.dll
    libgnutls-26.dll
    libgnutls-extra-26.dll
    libgnutls-openssl-26.dll
    libgnutlsxx-26.dll
    libgpg-error-0.dll
    libgstapp-0.10.dll
    libgstaudio-0.10.dll
    libgstbase-0.10.dll
    libgstcdda-0.10.dll
    libgstcontroller-0.10.dll
    libgstdataprotocol-0.10.dll
    libgstdshow-0.10.dll
    libgstfarsight-0.10.dll
    libgstfft-0.10.dll
    libgstgl-0.10.dll
    libgstinterfaces-0.10.dll
    libgstnet-0.10.dll
    libgstnetbuffer-0.10.dll
    libgstpbutils-0.10.dll
    libgstphotography-0.10.dll
    libgstreamer-0.10.dll
    libgstriff-0.10.dll
    libgstrtp-0.10.dll
    libgstrtsp-0.10.dll
    libgstsdp-0.10.dll
    libgsttag-0.10.dll
    libgstvideo-0.10.dll
    libjpeg.dll
    libmms-0.dll
    libmp3lame-0.dll
    libmpeg2-0.dll
    libmpeg2convert-0.dll
    libneon-27.dll
    libnice.dll
    libogg-0.dll
    liboil-0.3-0.dll
    libopenjpeg-2.dll
    libpango-1.0-0.dll
    libpangocairo-1.0-0.dll
    libpangoft2-1.0-0.dll
    libpangowin32-1.0-0.dll
    libpixman-1-0.dll
    libpng12-0.dll
    libschroedinger-1.0-0.dll
    libspeex-1.dll
    libspeexdsp-1.dll
    libtheora-0.dll
    libtheoradec-1.dll
    libtheoraenc-1.dll
    libvorbis-0.dll
    libvorbisenc-2.dll
    libvorbisfile-3.dll
    libwavpack-1.dll
    libx264-67.dll
    pthreadGC2.dll
    xvidcore.dll
    )

set(gstreamer_plugin_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-win32/lib/release/gstreamer/plugins")
set(gstreamer_plugin_files
libfsfunnel.dll
libfsrtcpfilter.dll
libfsrtpconference.dll
libfsvideoanyrate.dll
libgnl.dll
libgsta52dec.dll
libgstaacparse.dll
libgstadder.dll
libgstaiffparse.dll
libgstalaw.dll
libgstalpha.dll
libgstamrparse.dll
libgstapetag.dll
libgstappplugin.dll
libgstasfdemux.dll
libgstaudioconvert.dll
libgstaudiofx.dll
libgstaudiorate.dll
libgstaudioresample.dll
libgstaudiotestsrc.dll
libgstauparse.dll
libgstautoconvert.dll
libgstautodetect.dll
libgstavi.dll
libgstbayer.dll
libgstcairo.dll
libgstcamerabin.dll
libgstcdxaparse.dll
libgstcoreelements.dll
libgstcutter.dll
libgstdebug.dll
libgstdecodebin.dll
libgstdecodebin2.dll
libgstdeinterlace.dll
libgstdirectdraw.dll
libgstdirectsound.dll
libgstdshowdecwrapper.dll
libgstdshowsrcwrapper.dll
libgstdshowvideosink.dll
libgstdtmf.dll
libgstdts.dll
libgstdvdlpcmdec.dll
libgstdvdread.dll
libgstdvdspu.dll
libgstdvdsub.dll
libgsteffectv.dll
libgstequalizer.dll
libgstfaac.dll
libgstfaad.dll
libgstffmpegcolorspace.dll
libgstffmpeggpl.dll
libgstflv.dll
libgstflx.dll
libgstfreeze.dll
libgstfsselector.dll
libgstgdp.dll
libgstgio.dll
libgsth264parse.dll
libgsticydemux.dll
libgstid3demux.dll
libgstiec958.dll
libgstinterleave.dll
libgstjpeg.dll
libgstlame.dll
libgstlegacyresample.dll
libgstlevel.dll
libgstlibmms.dll
libgstliveadder.dll
libgstmatroska.dll
libgstmonoscope.dll
libgstmpeg2dec.dll
libgstmpeg4videoparse.dll
libgstmpegaudioparse.dll
libgstmpegdemux.dll
libgstmpegstream.dll
libgstmpegtsmux.dll
libgstmpegvideoparse.dll
libgstmulaw.dll
libgstmultifile.dll
libgstmultipart.dll
libgstmve.dll
libgstmxf.dll
libgstneon.dll
libgstnice.dll
libgstnuvdemux.dll
libgstogg.dll
libgstopengl.dll
libgstpango.dll
libgstpcapparse.dll
libgstplaybin.dll
libgstpng.dll
libgstqtdemux.dll
libgstqtmux.dll
libgstqueue2.dll
libgstrawparse.dll
libgstreal.dll
libgstrealmedia.dll
libgstreplaygain.dll
libgstresindvd.dll
libgstrtpjitterbuffer.dll
libgstrtpmanager.dll
libgstrtpmux.dll
libgstrtpnetsim.dll
libgstrtppayloads.dll
libgstrtprtpdemux.dll
libgstrtp_good.dll
libgstrtsp_good.dll
libgstscaletempo.dll
libgstsdpplugin.dll
libgstselector.dll
libgstsiren.dll
libgstsmpte.dll
libgstspectrum.dll
libgstspeed.dll
libgststereo.dll
libgstsubenc.dll
libgstsynaesthesia.dll
libgsttheora.dll
libgsttta.dll
libgsttypefindfunctions.dll
libgstudp.dll
libgstvalve.dll
libgstvideobox.dll
libgstvideocrop.dll
libgstvideofilterbalance.dll
libgstvideofilterflip.dll
libgstvideofiltergamma.dll
libgstvideomixer.dll
libgstvideorate.dll
libgstvideoscale.dll
libgstvideosignal.dll
libgstvideotestsrc.dll
libgstvmnc.dll
libgstvolume.dll
libgstvorbis.dll
libgstwasapi.dll
libgstwaveenc.dll
libgstwaveform.dll
libgstwavpack.dll
libgstwavparse.dll
libgstwininet.dll
libgstwinks.dll
libgstwinscreencap.dll
libgstx264.dll
libgstxvid.dll
libgsty4m.dll
    )

copy_if_different(
    ${gstreamer_plugin_src_dir} 
    "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/lib/gstreamer-0.10"
    out_targets 
    ${gstreamer_plugin_files}
    )
set(all_targets ${all_targets} ${out_targets})

copy_if_different(
    ${gstreamer_base_src_dir} 
    "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/"
    out_targets 
    ${gstreamer_base_files}
    )
set(all_targets ${all_targets} ${out_targets})

copy_if_different(
    ${gstreamer_plugin_src_dir} 
    "${CMAKE_CURRENT_BINARY_DIR}/Release/lib/gstreamer-0.10"
    out_targets 
    ${gstreamer_plugin_files}
    )
set(all_targets ${all_targets} ${out_targets})

copy_if_different(
    ${gstreamer_base_src_dir} 
    "${CMAKE_CURRENT_BINARY_DIR}/Release/"
    out_targets 
    ${gstreamer_base_files}
    )
set(all_targets ${all_targets} ${out_targets})

copy_if_different(
    ${gstreamer_plugin_src_dir} 
    "${CMAKE_CURRENT_BINARY_DIR}/Debug/lib/gstreamer-0.10"
    out_targets 
    ${gstreamer_plugin_files}
    )
set(all_targets ${all_targets} ${out_targets})
copy_if_different(
    ${gstreamer_base_src_dir} 
    "${CMAKE_CURRENT_BINARY_DIR}/Debug/"
    out_targets 
    ${gstreamer_base_files}
    )
set(all_targets ${all_targets} ${out_targets})

add_custom_target(copy_win_libs ALL DEPENDS ${all_targets})
