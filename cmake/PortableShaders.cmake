# Shader tools run on the host even while Emscripten cross-compiles the game
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(GLSLANG_EXECUTABLE NAMES glslangValidator glslang
    HINTS "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/Bin"
    NO_CMAKE_FIND_ROOT_PATH REQUIRED)
find_program(SPIRV_CROSS_EXECUTABLE NAMES spirv-cross
    HINTS "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/Bin"
    NO_CMAKE_FIND_ROOT_PATH REQUIRED)

#[[
@brief 共通HLSLから配布先に応じたシェーダーを生成する
@param target シェーダーへ依存する実行ターゲット
@return 親スコープのSPACEYAKUZA_SHADER_DIRに出力ディレクトリを設定する
]]
function(spaceyakuza_compile_shaders target)
    # 配布版ごとに必要な実行形式だけを含める
    set(mode vulkan)
    set(extension spv)
    if(Distribution STREQUAL "Web")
        set(mode web)
        set(extension glsl)
    endif()
    set(shader_dir "${CMAKE_CURRENT_BINARY_DIR}/Shaders")
    set(outputs)
    foreach(program object background spell shot explosion railgun font upscale)
        foreach(stage vert frag)
            list(APPEND outputs "${shader_dir}/${program}.${stage}.${extension}")
        endforeach()
    endforeach()
    set(script "${PROJECT_SOURCE_DIR}/Scripts/BuildPortableShaders.py")
    set(shader_args --source-root "${PROJECT_SOURCE_DIR}" --mode "${mode}"
        --glslang "${GLSLANG_EXECUTABLE}" --spirv-cross "${SPIRV_CROSS_EXECUTABLE}")
    add_custom_command(OUTPUT ${outputs}
        COMMAND "${Python3_EXECUTABLE}" "${script}" ${shader_args} --output "${shader_dir}"
        DEPENDS "${script}"
            "${PROJECT_SOURCE_DIR}/Shaders/ObjectShader.hlsl"
            "${PROJECT_SOURCE_DIR}/Shaders/BackgroundShader.hlsl"
            "${PROJECT_SOURCE_DIR}/Shaders/SpellCircleShader.hlsl"
            "${PROJECT_SOURCE_DIR}/Shaders/PixelUpscaleShader.hlsl"
            "${PROJECT_SOURCE_DIR}/Shaders/PortableFontShader.hlsl"
            "${PROJECT_SOURCE_DIR}/Infrastructure/ExternalServices/D3D12RenderingService.cpp"
        COMMENT "Compiling and validating ${Distribution} shaders" VERBATIM)
    add_custom_target(portable_shaders DEPENDS ${outputs})
    add_dependencies(${target} portable_shaders)
    # HLSLのみの変更でもWebのpreloadデータとMacの署名済みリソースを更新する
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS ${outputs})
    set(SPACEYAKUZA_SHADER_DIR "${shader_dir}" PARENT_SCOPE)
    set(SPACEYAKUZA_SHADER_FILES "${outputs}" PARENT_SCOPE)

    # 同じ小さな実行チェックをCTestからも利用する
    if(BUILD_TESTING)
        add_test(NAME portable_shader_contract
            COMMAND "${Python3_EXECUTABLE}" "${script}" ${shader_args} --check)
    endif()
endfunction()
