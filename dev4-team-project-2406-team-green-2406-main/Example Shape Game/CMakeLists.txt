cmake_minimum_required(VERSION 3.16)

project(ExampleShapeGame)

set(SOURCE_FILES
	main.cpp
	precompiled.h
	entities.h
	components.h
	systems.h
)
set(VERTEX_SHADERS
	DebugVS.hlsl
)
set(PIXEL_SHADERS
	DebugPS.hlsl
)

# currently using unicode in some libraries on win32 but will change soon
ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

# by default CMake selects "ALL_BUILD" as the startup project 
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
	 PROPERTY VS_STARTUP_PROJECT ExampleShapeGame)

add_executable (ExampleShapeGame ${SOURCE_FILES} ${VERTEX_SHADERS} ${PIXEL_SHADERS})

# EnTT requires c++17
target_compile_features(ExampleShapeGame PUBLIC cxx_std_17)

# adding Gateware and EnTT as a precompiled headers to reduce compile times
target_precompile_headers(ExampleShapeGame PRIVATE precompiled.h)

# shaders are set to dump precompiled versions next to themselves
set_source_files_properties( ${VERTEX_SHADERS} PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_MODEL 5.0
        VS_SHADER_ENTRYPOINT main
		VS_SHADER_OBJECT_FILE_NAME "%(RelativeDir)/%(Filename).cso"
)
set_source_files_properties( ${PIXEL_SHADERS} PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_MODEL 5.0
        VS_SHADER_ENTRYPOINT main
		VS_SHADER_OBJECT_FILE_NAME "%(RelativeDir)/%(Filename).cso"
)
