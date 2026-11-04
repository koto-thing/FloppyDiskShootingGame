# GPUを利用する検査は通常のユニット検査からラベルで分離する
if(BUILD_TESTING AND NOT EMSCRIPTEN)
    add_executable(PortableRendererSmoke Tests/PortableRendererSmoke.cpp
        Infrastructure/ExternalServices/PortableRenderingService.cpp)
    target_link_libraries(PortableRendererSmoke PRIVATE portable_sdl Vulkan::Vulkan)
    target_compile_definitions(PortableRendererSmoke PRIVATE SDL_MAIN_HANDLED NOMINMAX)
    if(MSVC)
        target_compile_options(PortableRendererSmoke PRIVATE /utf-8 /EHsc)
    endif()
    add_dependencies(PortableRendererSmoke portable_shaders)
    add_custom_command(TARGET PortableRendererSmoke POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${SPACEYAKUZA_SHADER_DIR}"
            "$<TARGET_FILE_DIR:PortableRendererSmoke>/Shaders")
    add_test(NAME PortableRendererSmoke COMMAND PortableRendererSmoke)
    set_tests_properties(PortableRendererSmoke PROPERTIES LABELS gpu TIMEOUT 60)
endif()
