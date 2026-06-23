#################################################
#
#  Standalone CMakeLists.txt for kcalc
#
#################################################

cmake_minimum_required( VERSION 3.5 )
project( calc )

##### Package setup #############################
include( FindPkgConfig )
include( TDEMacros )
find_package( TQt REQUIRED )
find_package( TDE REQUIRED )
find_package( X11 REQUIRED )
pkg_search_module( FONTCONFIG REQUIRED fontconfig )
include( TDESetupPaths )

tde_setup_paths( )

set(COMPRESS "zx0" CACHE STRING "Compression method (zx0, lz4 or zlib)")
set(ZLIB_LIBS "")
add_definitions(-DUSE_ZX0)
set(DECOMPRESSOR_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/decompressors/zx0/zx0_decompress.c")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/decompressors/zx0")

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/knumber
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${FONTCONFIG_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${TDE_LIB_DIR}
)

##### configure checks ##########################
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)

check_type_size( "unsigned long" SIZEOF_UNSIGNED_LONG )
check_include_file( "stdlib.h" HAVE_STDLIB_H )
check_include_file( "ieeefp.h" HAVE_IEEEFP_H )
check_type_size( "long double" SIZEOF_LONG_DOUBLE )
if( SIZEOF_LONG_DOUBLE )
  set( HAVE_LONG_DOUBLE 1 )
endif( )

set( CMAKE_REQUIRED_LIBRARIES m )
check_symbol_exists( "fabsl"  "math.h" HAVE_L_FUNCS )
check_symbol_exists( "isinf"  "math.h" HAVE_FUNC_ISINF )
check_symbol_exists( "round"  "math.h" HAVE_FUNC_ROUND )
check_symbol_exists( "roundl" "math.h" HAVE_FUNC_ROUNDL )

set( CMAKE_REQUIRED_LIBRARIES gmp )
check_symbol_exists( "gmp_asprintf" "gmp.h" HAVE_GMPLIB )
set( CMAKE_REQUIRED_LIBRARIES )

if( HAVE_GMPLIB )
  set( GMP_LIBRARY gmp CACHE INTERNAL "" )
else( )
  message( FATAL_ERROR "libgmp is required, but was not found on your system" )
endif( )

add_definitions(
  -D_GNU_SOURCE
  -D_ISOC99_SOURCE
  -DTDE_EXPORT=KDE_EXPORT
  -DTQ_SIGNAL=TQT_SIGNAL
  -DTQ_SLOT=TQT_SLOT
  -DtdeApp=kapp
)

##### generate config.h #########################
configure_file( config.h.cmake config.h @ONLY )

##### compiler flags #######################################
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "-Oz -flto=auto -fstrict-aliasing -ffunction-sections -fdata-sections -fno-ident -fno-plt -fomit-frame-pointer -fno-stack-protector -fno-math-errno -fvisibility=hidden -fno-threadsafe-statics -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -Wno-deprecated-declarations -DQT_SHARED -DQT_TABLET_SUPPORT -DTQT_NO_DEBUG -DQT_NO_EXCEPTIONS -DTQT_THREAD_SUPPORT -D_REENTRANT -fpermissive -std=gnu++17")
    set(CMAKE_C_FLAGS "-Oz -flto=auto -fstrict-aliasing -ffunction-sections -fdata-sections -fno-ident -fno-plt -fomit-frame-pointer -fno-stack-protector -fno-math-errno -fvisibility=hidden -Wno-deprecated-declarations -std=gnu17")
else()
    set(CMAKE_CXX_FLAGS "-Os -flto=auto -fstrict-aliasing -ffunction-sections -fdata-sections -fno-ident -fno-plt -fomit-frame-pointer -fno-stack-protector -fno-math-errno -fvisibility=hidden -fno-threadsafe-statics -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -Wno-deprecated-declarations -DQT_SHARED -DQT_TABLET_SUPPORT -DTQT_NO_DEBUG -DQT_NO_EXCEPTIONS -DTQT_THREAD_SUPPORT -D_REENTRANT -fpermissive -std=gnu++17")
    set(CMAKE_C_FLAGS "-Os -flto=auto -fstrict-aliasing -ffunction-sections -fdata-sections -fno-ident -fno-plt -fomit-frame-pointer -fno-stack-protector -fno-math-errno -fvisibility=hidden -Wno-deprecated-declarations -std=gnu17")
endif()
set(CMAKE_EXE_LINKER_FLAGS "-flto=auto -Wl,-O1 -Wl,--gc-sections -Wl,--as-needed -s")

# Ensure no per-config C++ flags are appended by CMake.
set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "")
set(CMAKE_CXX_FLAGS_MINSIZEREL "")
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "")
set(CMAKE_C_FLAGS_MINSIZEREL "")

find_program(SSSTRIP_EXECUTABLE NAMES sstrip)

##### Custom generation for kcfg ################
# We handle this manually to avoid target-dependency cycles in Ninja
# when building as a standalone top-level CMake project.
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/calc_settings.cpp ${CMAKE_CURRENT_BINARY_DIR}/calc_settings.h
  COMMAND ${KDE3_KCFGC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/src/calc.kcfg ${CMAKE_CURRENT_SOURCE_DIR}/src/calc_settings.kcfgc
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/calc.kcfg ${CMAKE_CURRENT_SOURCE_DIR}/src/calc_settings.kcfgc
)

add_subdirectory( src/knumber )

##### Embedded Assets ##########################################
file(GLOB ICON_PNGS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*.png")
file(GLOB FONT_TTFS "${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.ttf")
file(GLOB TXT_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/translations/*.txt")

set(ASSET_POOL_DIR "${CMAKE_CURRENT_BINARY_DIR}/asset_pool")

add_custom_command(
    OUTPUT "${ASSET_POOL_DIR}/.copied"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ASSET_POOL_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy ${ICON_PNGS} ${FONT_TTFS} ${TXT_ASSETS} "${ASSET_POOL_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E touch "${ASSET_POOL_DIR}/.copied"
    DEPENDS ${ICON_PNGS} ${FONT_TTFS} ${TXT_ASSETS}
    COMMENT "Preparing asset pool"
)

# Host tool to compress assets (built from copied zx0em sources)
set(ZX0EM_COMPRESSOR "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/binaries/zx0em_pack_x86_64_v4")
if(EXISTS "${ZX0EM_COMPRESSOR}")
    message(STATUS "Using precompiled optimized zx0em compressor: ${ZX0EM_COMPRESSOR}")
    set(ZX0EM_COMPRESSOR_DEP "")
else()
    message(STATUS "Precompiled zx0em compressor not found, compiling from source")
    set(ZX0EM_COMPRESSOR "$<TARGET_FILE:bin_compress>")
    set(ZX0EM_COMPRESSOR_DEP bin_compress)
endif()

add_executable(bin_compress EXCLUDE_FROM_ALL
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/zx0em_pack.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/blob_builder.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/emit_header.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/png_decode.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/similarity.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/preprocess.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/text_bpe.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/ttf_strip.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/zx0/compress.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/zx0/optimize.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/zx0/memory.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/zx0/zx0_compress_wrapper.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/salvador/src/expand.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/salvador/src/matchfinder.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/salvador/src/shrink.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/salvador/src/libdivsufsort/lib/divsufsort.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/salvador/src/libdivsufsort/lib/divsufsort_utils.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/salvador/src/libdivsufsort/lib/sssort.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/salvador/src/libdivsufsort/lib/trsort.c"
)
target_include_directories(bin_compress PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/zx0"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/salvador/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zx0em/salvador/src/libdivsufsort/include"
)
target_link_libraries(bin_compress m pthread)

add_custom_command(
    OUTPUT
         "${CMAKE_CURRENT_BINARY_DIR}/embedded_images_data.h"
         "${CMAKE_CURRENT_BINARY_DIR}/embedded_images_data.cpp"
         "${CMAKE_CURRENT_BINARY_DIR}/embedded_icons.h"
         "${CMAKE_CURRENT_BINARY_DIR}/embedded_fonts.h"
         "${CMAKE_CURRENT_BINARY_DIR}/embedded_txt.h"
         "${CMAKE_CURRENT_BINARY_DIR}/embedded_assets_compat.cpp"
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_assets.py" "${ASSET_POOL_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" "${ZX0EM_COMPRESSOR}" "${COMPRESS}"
    DEPENDS "${ASSET_POOL_DIR}/.copied" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_assets.py" ${ZX0EM_COMPRESSOR_DEP}
    COMMENT "Compressing and packaging embedded assets"
)

add_custom_target(embedded_assets DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/embedded_images_data.h"
    "${CMAKE_CURRENT_BINARY_DIR}/embedded_images_data.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/embedded_icons.h"
    "${CMAKE_CURRENT_BINARY_DIR}/embedded_fonts.h"
    "${CMAKE_CURRENT_BINARY_DIR}/embedded_txt.h"
    "${CMAKE_CURRENT_BINARY_DIR}/embedded_assets_compat.cpp"
)

# Disable tdelfeditor to prevent harmless but annoying errors about ELF resources and missing icons
set(_TDELFEDITOR_EXECUTABLE ${TDELFEDITOR_EXECUTABLE})
set(TDELFEDITOR_EXECUTABLE "")

##### kcalc (standalone executable) ###########################
tde_add_executable( calc AUTOMOC
  SOURCES src/calc.cpp src/calc_button.cpp src/history_window.cpp
    src/calc_const_button.cpp src/calc_const_menu.cpp
    src/calc_core.cpp src/calcdisplay.cpp src/dlabel.cpp
    src/stats.cpp calc_settings.cpp src/settings_dialog.cpp
    src/tqtlcdwidget.cpp
    src/tqtdateperiodpicker.cpp
    src/tqtdateperiodpickercalendarpanel.cpp
    src/tqtdateperiodpickerdecadeview.cpp
    src/tqtdateperiodpickerformatter.cpp
    src/tqtdateperiodpickermonthview.cpp
    src/tqtdateperiodpickernavigator.cpp
    src/tqtdateperiodpickerpopup.cpp
    src/tqtdateperiodpickerpopupfooter.cpp
    src/tqtdateperiodpickeryearview.cpp
    src/tqtembeddedimages.cpp
    ${DECOMPRESSOR_SRC}
    ${CMAKE_CURRENT_BINARY_DIR}/embedded_images_data.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/embedded_assets_compat.cpp
  LINK ${TQT_LIBRARIES} knumber-static tdecore tdeui
)

# Restore tdelfeditor
set(TDELFEDITOR_EXECUTABLE ${_TDELFEDITOR_EXECUTABLE})

target_link_libraries( calc ${X11_LIBRARIES} ${FONTCONFIG_LIBRARIES} ${ZLIB_LIBS} )

add_dependencies(calc embedded_assets)

if (SSSTRIP_EXECUTABLE)
    add_custom_command(TARGET calc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:calc> $<TARGET_FILE:calc>.tmp
        COMMAND ${SSSTRIP_EXECUTABLE} $<TARGET_FILE:calc>.tmp
        COMMAND ${CMAKE_COMMAND} -E rename $<TARGET_FILE:calc>.tmp $<TARGET_FILE:calc>
        VERBATIM
    )
endif()

##### other data ################################
install( FILES src/calcui.rc
  DESTINATION ${DATA_INSTALL_DIR}/calc
)

tde_create_translated_desktop( calc.desktop )

install( FILES src/calcupd.upd
  DESTINATION ${KCONF_UPDATE_INSTALL_DIR}
)

install( FILES src/calc.kcfg
  DESTINATION ${KCFG_INSTALL_DIR}
)
