cmake_minimum_required(VERSION 3.24)
project(SpaceYakuza LANGUAGES C CXX)

# ゲーム機能を配布版で選び、OSと描画APIを別々に検証する
set(Distribution "" CACHE STRING "Mac, Linux or Web")
set_property(CACHE Distribution PROPERTY STRINGS Mac Linux Web)
option(SPACEYAKUZA_HOST_SMOKE "Build the Linux Vulkan path on Windows for local validation only" OFF)
if(NOT Distribution MATCHES "^(Mac|Linux|Web)$")
    message(FATAL_ERROR "Use -DDistribution=Mac, Linux or Web; Windows editions use the existing MSBuild projects")
endif()
if(Distribution STREQUAL "Web" AND NOT EMSCRIPTEN)
    message(FATAL_ERROR "The Web edition requires emcmake / the Emscripten toolchain")
elseif(Distribution STREQUAL "Mac" AND NOT APPLE)
    message(FATAL_ERROR "Build the Mac edition on macOS with Xcode command line tools")
elseif(Distribution STREQUAL "Linux" AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT (WIN32 AND SPACEYAKUZA_HOST_SMOKE))
    message(FATAL_ERROR "Build the Linux edition on Linux")
elseif(EMSCRIPTEN AND NOT Distribution STREQUAL "Web")
    message(FATAL_ERROR "Emscripten can only build Distribution=Web")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)

if(EMSCRIPTEN)
    add_library(portable_sdl INTERFACE)
    target_compile_options(portable_sdl INTERFACE -sUSE_SDL=2)
    target_link_options(portable_sdl INTERFACE -sUSE_SDL=2)
else()
    option(SPACEYAKUZA_SYSTEM_SDL "Use an installed SDL2 instead of bundling the pinned static library" OFF)
    if(SPACEYAKUZA_SYSTEM_SDL)
        find_package(SDL2 2.0.18 CONFIG REQUIRED)
    endif()
    if(NOT TARGET SDL2::SDL2 AND NOT TARGET SDL2::SDL2-static)
        include(FetchContent)
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
        FetchContent_Declare(SDL2 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG 5d249570393f7a37e037abf22cd6012a4cc56a71)
        FetchContent_MakeAvailable(SDL2)
    endif()
    add_library(portable_sdl INTERFACE)
    if(TARGET SDL2::SDL2-static)
        target_link_libraries(portable_sdl INTERFACE SDL2::SDL2-static)
    else()
        target_link_libraries(portable_sdl INTERFACE SDL2::SDL2)
    endif()
    if(APPLE)
        find_path(Vulkan_INCLUDE_DIR vulkan/vulkan.h HINTS "$ENV{VULKAN_SDK}/include" REQUIRED)
        find_library(MOLTENVK_LIBRARY NAMES MoltenVK HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)
        add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
        set_target_properties(Vulkan::Vulkan PROPERTIES IMPORTED_LOCATION "${MOLTENVK_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIR}")
    else()
        find_package(Vulkan REQUIRED)
    endif()
endif()

# 既存プロジェクトのソース一覧を再利用してステージ追加時の二重管理を防ぐ
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/FloppyEngine.vcxproj"
    "${CMAKE_CURRENT_SOURCE_DIR}/FloppyDiskShootingGame.vcxproj")
set(game_sources)
foreach(project_file FloppyEngine.vcxproj FloppyDiskShootingGame.vcxproj)
    file(STRINGS "${project_file}" source_lines REGEX "<ClCompile Include=")
    foreach(line IN LISTS source_lines)
        string(REGEX REPLACE ".*Include=\"([^\"]+)\".*" "\\1" source "${line}")
        string(REPLACE "\\" "/" source "${source}")
        list(APPEND game_sources "${source}")
    endforeach()
endforeach()
list(REMOVE_DUPLICATES game_sources)
list(FILTER game_sources EXCLUDE REGEX "(^main.cpp$|/(D3D12RenderingService|TextRenderingService|Win32WindowService|WindowsInputBackend|Switch2ProInput|InputService|MeshRenderer|AudioService)\\.cpp$)")
list(APPEND game_sources Engine/Input/SdlInputBackend.cpp Infrastructure/ExternalServices/SdlAudioService.cpp)
add_library(spaceyakuza_game STATIC ${game_sources})
target_link_libraries(spaceyakuza_game PUBLIC portable_sdl)
target_compile_definitions(spaceyakuza_game PUBLIC SPACEYAKUZA_PORTABLE=1 SPACEYAKUZA_EDITION_${Distribution}=1
    NOMINMAX SDL_MAIN_HANDLED $<$<CONFIG:Debug>:_DEBUG>)
target_include_directories(spaceyakuza_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/generated")
if(MSVC)
    target_compile_options(spaceyakuza_game PUBLIC /utf-8 /EHsc)
else()
    target_compile_options(spaceyakuza_game PUBLIC -fexceptions)
endif()

# 版ごとの既存カウンター形式を共有し、ビルドごとに採番する
add_custom_target(portable_version
    COMMAND "${CMAKE_COMMAND}" -DDistribution=${Distribution}
        "-DCounterDirectory=${CMAKE_CURRENT_SOURCE_DIR}/.buildnumbers"
        "-DHeaderPath=${CMAKE_CURRENT_BINARY_DIR}/generated/BuildVersion.h"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateBuildVersion.cmake"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/generated/BuildVersion.h" VERBATIM)
add_dependencies(spaceyakuza_game portable_version)
add_executable(SpaceYakuza main_portable.cpp Infrastructure/ExternalServices/PortableRenderingService.cpp)
target_link_libraries(SpaceYakuza PRIVATE spaceyakuza_game)
if(NOT EMSCRIPTEN)
    target_link_libraries(SpaceYakuza PRIVATE Vulkan::Vulkan)
endif()
include(cmake/PortableShaders.cmake)
spaceyakuza_compile_shaders(SpaceYakuza)

# 同梱するランタイム依存のライセンスだけを配布する
set(portable_licenses "${PROJECT_SOURCE_DIR}/Resources/PortableLicenses/SDL2.txt"
    "${PROJECT_SOURCE_DIR}/Resources/PortableLicenses/Sources.txt")
if(APPLE)
    list(APPEND portable_licenses "${PROJECT_SOURCE_DIR}/Resources/PortableLicenses/MoltenVK-LICENSE.txt")
endif()
set_property(TARGET SpaceYakuza APPEND PROPERTY LINK_DEPENDS ${portable_licenses})

if(EMSCRIPTEN)
    set_target_properties(SpaceYakuza PROPERTIES SUFFIX ".html")
    target_link_options(SpaceYakuza PRIVATE -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
        -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=8388608 -sINITIAL_MEMORY=67108864
        -sFORCE_FILESYSTEM=1 -sEXIT_RUNTIME=0 -sDISABLE_EXCEPTION_CATCHING=0 -lidbfs.js
        "-sEXPORTED_RUNTIME_METHODS=['callMain']"
        "--pre-js=${CMAKE_CURRENT_SOURCE_DIR}/Resources/WebStorage.js"
        "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/Resources/WebShell.html"
        "--preload-file=${SPACEYAKUZA_SHADER_DIR}@/Shaders")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SpaceYakuza.html" "${CMAKE_CURRENT_BINARY_DIR}/SpaceYakuza.js"
        "${CMAKE_CURRENT_BINARY_DIR}/SpaceYakuza.wasm" "${CMAKE_CURRENT_BINARY_DIR}/SpaceYakuza.data" DESTINATION .)
else()
    if(APPLE)
        set_target_properties(SpaceYakuza PROPERTIES MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.kotothing.spaceyakuza" MACOSX_BUNDLE_BUNDLE_NAME "Space Yakuza")
        set(shader_destination "$<TARGET_BUNDLE_DIR:SpaceYakuza>/Contents/Resources/Shaders")
        install(TARGETS SpaceYakuza BUNDLE DESTINATION .)
    else()
        set(shader_destination "$<TARGET_FILE_DIR:SpaceYakuza>/Shaders")
        install(TARGETS SpaceYakuza RUNTIME DESTINATION .)
        install(FILES ${SPACEYAKUZA_SHADER_FILES} DESTINATION Shaders)
    endif()
    add_custom_command(TARGET SpaceYakuza POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${SPACEYAKUZA_SHADER_DIR}" "${shader_destination}" VERBATIM)
    if(APPLE)
        add_custom_command(TARGET SpaceYakuza POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_DIR:SpaceYakuza>/Contents/Resources/Licenses"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${portable_licenses}
                "$<TARGET_BUNDLE_DIR:SpaceYakuza>/Contents/Resources/Licenses" VERBATIM)
        add_custom_command(TARGET SpaceYakuza POST_BUILD COMMAND "${CMAKE_COMMAND}"
            "-DBUNDLE=$<TARGET_BUNDLE_DIR:SpaceYakuza>" "-DMOLTENVK_LIBRARY=${MOLTENVK_LIBRARY}"
            -P "${PROJECT_SOURCE_DIR}/cmake/BundleMac.cmake" VERBATIM)
    endif()
endif()
if(NOT APPLE)
    install(FILES ${portable_licenses} DESTINATION Licenses)
endif()

if(BUILD_TESTING AND NOT EMSCRIPTEN)
    add_executable(PortablePlatformTests Tests/PortablePlatformTests.cpp)
    target_link_libraries(PortablePlatformTests PRIVATE spaceyakuza_game)
    target_compile_definitions(spaceyakuza_game PRIVATE FLOPPY_AUDIO_TESTS)
    if(MSVC)
        target_compile_options(spaceyakuza_game PRIVATE /UNDEBUG)
        target_compile_options(PortablePlatformTests PRIVATE /UNDEBUG)
    else()
        target_compile_options(spaceyakuza_game PRIVATE -UNDEBUG)
        target_compile_options(PortablePlatformTests PRIVATE -UNDEBUG)
    endif()
    add_test(NAME portable_platform COMMAND PortablePlatformTests)
    # 既存のゲーム回帰チェックを新しいOSでもそのまま実行する
    foreach(test CooperativeSetup HomingShot OrbitShot StageSelect CoopGameplay)
        add_executable(${test}Tests "Tests/${test}Tests.cpp")
        if(test STREQUAL "CoopGameplay")
            target_sources(${test}Tests PRIVATE Tests/CoopStageTests.cpp)
        endif()
        target_link_libraries(${test}Tests PRIVATE spaceyakuza_game)
        if(MSVC)
            target_compile_options(${test}Tests PRIVATE /UNDEBUG)
        else()
            target_compile_options(${test}Tests PRIVATE -UNDEBUG)
        endif()
        add_test(NAME ${test} COMMAND ${test}Tests)
    endforeach()
endif()
if(BUILD_TESTING)
    include(cmake/PortableRendererTests.cmake OPTIONAL)
endif()
set(CPACK_PACKAGE_NAME "SpaceYakuza-${Distribution}")
set(CPACK_PACKAGE_VERSION "1.0")
set(CPACK_GENERATOR "ZIP")
if(SPACEYAKUZA_HOST_SMOKE)
    # Windows上の検査用exeをLinuxの配布物として誤って梱包しない
    set(CPACK_PACKAGE_NAME "SpaceYakuza-HostSmoke-NotForDistribution")
    set(CPACK_PROJECT_CONFIG_FILE "${PROJECT_SOURCE_DIR}/cmake/RejectHostSmokePackage.cmake")
endif()
include(CPack)
