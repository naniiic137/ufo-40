# UFO 40 - CMake build for PS Vita (vitasdk), the web (Emscripten) and
# native desktop / CI. The source list is shared with the Makefile via
# sources.mk, so there is exactly one list of files.
#
#   Vita:    cmake -S . -B build-vita -DUFO40_VITA=ON && cmake --build build-vita
#   Web:     emcmake cmake -S . -B build-web && cmake --build build-web
#   Desktop: cmake -S . -B build-cmake && cmake --build build-cmake && ctest --test-dir build-cmake

cmake_minimum_required(VERSION 3.16)

option(UFO40_VITA "Build the PS Vita .vpk (needs VITASDK)" OFF)

if(UFO40_VITA AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(ufo40 C)

# ---- read the shared source list -------------------------------------------
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/sources.mk" _ufo40_lines)
foreach(_line IN LISTS _ufo40_lines)
  if(_line MATCHES "^([A-Z]+)_SRC \\+= (.+)$")
    list(APPEND UFO40_${CMAKE_MATCH_1}_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_MATCH_2}")
  endif()
endforeach()
if(NOT UFO40_CORE_SRC OR NOT UFO40_SDL_SRC OR NOT UFO40_HEADLESS_SRC)
  message(FATAL_ERROR "could not parse sources.mk")
endif()

# ---- PS Vita ------------------------------------------------------------------
if(UFO40_VITA)
  include("${VITASDK}/share/vita.cmake" REQUIRED)
  find_package(SDL2 REQUIRED)

  set(VITA_APP_NAME "UFO 40")
  set(VITA_TITLEID  "UFOF00040")
  set(VITA_VERSION  "01.00")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -O2 -Wall -Wno-unused-parameter")

  # vitasdk's static SDL2 contains C++ code, so link with g++ to pull in libstdc++.
  enable_language(CXX)
  add_executable(ufo40 ${UFO40_CORE_SRC} ${UFO40_SDL_SRC})
  set_target_properties(ufo40 PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(ufo40 SDL2::SDL2 ScePower_stub stdc++ m)
  # SDL2's GL path (vitashark) also references the runtime shader compiler. We
  # never use it (the GXM renderer is forced in main_sdl.c), but the linker
  # still needs it, and it has to come after SDL2's own dependencies, which
  # CMake appends last. The standard-libraries slot is the very end of the line.
  set(CMAKE_CXX_STANDARD_LIBRARIES
      "${CMAKE_CXX_STANDARD_LIBRARIES} -lSceShaccCgExt -ltaihen_stub -lSceShaccCg_stub -lstdc++ -lm")

  vita_create_self(ufo40.self ufo40)
  vita_create_vpk(ufo40.vpk ${VITA_TITLEID} ufo40.self
    VERSION ${VITA_VERSION}
    NAME "${VITA_APP_NAME}"
    FILE vita/sce_sys/icon0.png sce_sys/icon0.png
    FILE vita/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE vita/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE vita/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  )
  return()
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---- Web (Emscripten, SDL2 port) --------------------------------------------------
if(EMSCRIPTEN)
  add_executable(ufo40 ${UFO40_CORE_SRC} ${UFO40_SDL_SRC})
  set_target_properties(ufo40 PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")
  target_compile_options(ufo40 PRIVATE -O2 -sUSE_SDL=2)
  target_link_options(ufo40 PRIVATE
    -O2
    -sUSE_SDL=2
    -sALLOW_MEMORY_GROWTH=1
    -sEXPORTED_FUNCTIONS=_main,_ufo_web_button
    --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
  set_property(TARGET ufo40 APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
  return()
endif()

# ---- Desktop / CI -----------------------------------------------------------------
if(NOT MSVC)
  set(UFO40_LIBM m)
endif()

add_executable(ufo40_headless ${UFO40_CORE_SRC} ${UFO40_HEADLESS_SRC})
target_link_libraries(ufo40_headless ${UFO40_LIBM})

enable_testing()
file(GLOB UFO40_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.ufs")
foreach(_t IN LISTS UFO40_TESTS)
  get_filename_component(_n "${_t}" NAME_WE)
  add_test(NAME ${_n}
    COMMAND ufo40_headless --script "${_t}"
            --save-dir "${CMAKE_CURRENT_BINARY_DIR}/testsave/${_n}"
            --out "${CMAKE_CURRENT_BINARY_DIR}/testshots")
endforeach()

find_package(SDL2 QUIET)
if(SDL2_FOUND)
  add_executable(ufo40 ${UFO40_CORE_SRC} ${UFO40_SDL_SRC})
  if(TARGET SDL2::SDL2main)
    target_link_libraries(ufo40 SDL2::SDL2main)
  endif()
  target_link_libraries(ufo40 SDL2::SDL2 ${UFO40_LIBM})
else()
  message(STATUS "SDL2 not found: building only the headless runner")
endif()
