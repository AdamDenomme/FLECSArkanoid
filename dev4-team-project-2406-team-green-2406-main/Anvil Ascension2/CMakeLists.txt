cmake_minimum_required(VERSION 3.16)

project(Anvil_Ascension)

# CMake FXC shader compilation, add any shaders you want compiled here
set(VERTEX_SHADERS 
    # add vertex shader (.hlsl) files here
    ${CMAKE_SOURCE_DIR}/Shaders/VertexShader.glsl
)

set(PIXEL_SHADERS 
    # add pixel shader (.hlsl) files here
    ${CMAKE_SOURCE_DIR}/Shaders/FragmentShader.glsl
)

set_source_files_properties(./flecs-3.1.4/flecs.c PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

if(WIN32)
    # by default CMake selects "ALL_BUILD" as the startup project
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
        PROPERTY VS_STARTUP_PROJECT Anvil_Ascension)

    #this will tell Visual Studio to put the shaders under their own group in the solution explorer 
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${VERTEX_SHADERS})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PIXEL_SHADERS})
endif()

# currently using unicode in some libraries on win32 but will change soon
add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)

add_executable (Anvil_Ascension 
                 main.cpp
                 load_data_oriented.h
                 load_object_oriented.h
		components.h
		FileIntoString.h
		gameplay.h
		h2bParser.h
		Menus.h
		OpenGLExtensions.h
		Physics.h
		stb_image.h
		flecs-3.2.0/flecs.c
		imgui-master/imgui.cpp
		imgui-master/imgui_demo.cpp
		imgui-master/imgui_draw.cpp
		imgui-master/imgui_widgets.cpp
		imgui-master/imgui_tables.cpp
		imgui-master/backends/imgui_impl_opengl3.h
		imgui-master/backends/imgui_impl_opengl3.cpp
		imgui-master/imgui.h

    ${VERTEX_SHADERS}
    ${PIXEL_SHADERS}
)
