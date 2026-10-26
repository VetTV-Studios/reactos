# SPDX-License-Identifier: GPL-2.0-or-later
#
# vettvshell.exe - VetTV Retro OS game launcher (M4). Spec:
# lanes/shell-ux/SHELL.md on the team box.
#
# Not pulled in by modules/CMakeLists.txt yet. Build & Kernel adds the
# add_subdirectory line and the installer entry (%SystemDrive%\VetTV\shell
# plus the HKLM Run value VetTVShell in hivesft.inf).

add_executable(vettvshell
    src/main.c
    src/classify.c
    src/library.c
    src/launch.c
    src/paths.c)
target_include_directories(vettvshell PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
# No _WIN32_WINNT/WINVER here: the tree already passes 0x502 and a second
# spelling is a -Werror redefinition (see 86a0fdf776).
set_module_type(vettvshell win32gui UNICODE)
add_importlibs(vettvshell user32 gdi32 shell32 msvcrt kernel32)
add_dependencies(vettvshell psdk)
# VetTV: always shipped (default image too). Uncompressed on the CD (i386\VetTV\shell); the
# generated txtsetup.sif maps it to C:\VetTV\shell\ (modules/vettv/payload/payload.cmake).
add_cd_file(TARGET vettvshell DESTINATION reactos/VetTV/shell NO_CAB FOR bootcd regtest)
