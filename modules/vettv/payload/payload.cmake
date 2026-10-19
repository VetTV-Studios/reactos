# SPDX-License-Identifier: GPL-2.0-or-later
#
# VetTV: install-time layout under %SystemDrive%\VetTV\ (shell + optional QA payload).
#
# Included from boot/bootdata/CMakeLists.txt. Sets VETTV_TXTSETUP_SIF, the txtsetup.sif
# that goes on the CD (stock file plus VetTV lines appended).
#
# VetTV: why txtsetup.sif and not add_cd_file(... DESTINATION VetTV/...) into reactos.cab:
#  - reactos.cab can only target the fixed dir_to_num() ids (all relative to C:\ReactOS),
#    and cabman keys files by basename. WineVDM ships same-named stubs in several folders
#    (dummydll\SYSTEM vs WINDOWS\SYSTEM: USER.EXE, GDI.EXE, ...) and names that clash with
#    ReactOS files (README.txt, winhlp32.exe, WINSPOOL.DRV), so they cannot share the cab.
#  - usetup copies every [SourceDisksFiles] line from the CD as-is (per line, with a
#    source subdirectory), and its [Directories] accept absolute "\..." paths, which
#    usetup creates under the install drive root. So the files go uncompressed on the CD
#    under i386\VetTV\... (NO_CAB) and txtsetup.sif maps them to C:\VetTV\....
#  - The INF parser merges repeated section headers, so we only append.
#
# VETTV_PAYLOAD_DIR (empty by default): staging folder holding dosbox-x/ and winevdm/
# (see lanes/build-kernel/payload on the team box). Never commit those binaries.

# VetTV: always included; the shell lines are always appended, the payload only when set.
set(VETTV_PAYLOAD_DIR "" CACHE PATH "VetTV: optional QA payload dir with dosbox-x/ and winevdm/ (empty = none)")

set(_vt_stock_sif ${REACTOS_SOURCE_DIR}/boot/bootdata/txtsetup.sif)
set(VETTV_TXTSETUP_SIF ${CMAKE_BINARY_DIR}/boot/bootdata/vettv/txtsetup.sif)
file(READ ${_vt_stock_sif} _vt_sif)

# VetTV: directory ids 100+ are ours; parents first so usetup can create them in order.
set(_vt_dirs "\n; VetTV: appended by modules/vettv/payload/payload.cmake\n[Directories]\n100 = \"\\VetTV\"\n101 = \"\\VetTV\\shell\"\n")
# VetTV: vettvshell.exe -> C:\VetTV\shell\ (Shell/UX spec; HKLM Run value is in hivesft.inf).
set(_vt_files "\n[SourceDisksFiles]\nvettvshell.exe = 1,VetTV\\shell,,,,,,101,,,\n")
set(_vt_next_id 102)

if(VETTV_PAYLOAD_DIR)
    if(NOT EXISTS "${VETTV_PAYLOAD_DIR}/dosbox-x/dosbox-x.exe" OR NOT EXISTS "${VETTV_PAYLOAD_DIR}/winevdm/otvdmw.exe")
        message(FATAL_ERROR "VetTV: VETTV_PAYLOAD_DIR=${VETTV_PAYLOAD_DIR} needs dosbox-x/dosbox-x.exe and winevdm/otvdmw.exe")
    endif()
    message(STATUS "VetTV: QA payload from ${VETTV_PAYLOAD_DIR}")
    foreach(_vt_top dosbox-x winevdm)
        file(GLOB_RECURSE _vt_list LIST_DIRECTORIES TRUE RELATIVE ${VETTV_PAYLOAD_DIR} ${VETTV_PAYLOAD_DIR}/${_vt_top}/*)
        set(_vt_subdirs ${_vt_top})
        foreach(_p ${_vt_list})
            if(IS_DIRECTORY ${VETTV_PAYLOAD_DIR}/${_p})
                list(APPEND _vt_subdirs ${_p})
            endif()
        endforeach()
        list(SORT _vt_subdirs) # parents sort before children
        foreach(_d ${_vt_subdirs})
            string(REPLACE "/" "\\" _dw "${_d}")
            set(_vt_dirs "${_vt_dirs}${_vt_next_id} = \"\\VetTV\\${_dw}\"\n")
            file(GLOB _vt_entries LIST_DIRECTORIES FALSE ${VETTV_PAYLOAD_DIR}/${_d}/*)
            foreach(_f ${_vt_entries})
                if(IS_DIRECTORY ${_f})
                    continue()
                endif()
                get_filename_component(_n ${_f} NAME)
                set(_vt_files "${_vt_files}${_n} = 1,VetTV\\${_dw},,,,,,${_vt_next_id},,,\n")
                # VetTV: uncompressed on the CD at i386\VetTV\<dir>\ (see header for why not the cab).
                add_cd_file(FILE ${_f} DESTINATION reactos/VetTV/${_d} NO_CAB FOR bootcd regtest)
            endforeach()
            math(EXPR _vt_next_id "${_vt_next_id} + 1")
        endforeach()
    endforeach()
endif()

file(WRITE ${VETTV_TXTSETUP_SIF} "${_vt_sif}${_vt_dirs}${_vt_files}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_vt_stock_sif})
