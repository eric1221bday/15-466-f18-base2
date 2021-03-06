cmake_minimum_required(VERSION 3.9)

project(indirect-pong)

set(CMAKE_CXX_STANDARD 14)

find_package(OpenGL REQUIRED)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

	set(KIT_DIR "${CMAKE_SOURCE_DIR}/kit-libs-win")
	set(SDL2_INCLUDE_DIRS "${KIT_DIR}/out/include/SDL2")
	set(SDL2_LIBRARIES "${KIT_DIR}/out/lib/SDL2.lib" "${KIT_DIR}/out/lib/SDL2main.lib")
	set(PNG_INCLUDE_DIRS "${KIT_DIR}/out/libpng")
	set(PNG_LIBRARIES "${KIT_DIR}/out/libpng/libpng.lib" "${KIT_DIR}/out/zlib/zlib.lib")
	set(GLM_INCLUDE_DIRS "${KIT_DIR}/out/include")
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /wd4146 /wd4297")

else ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

	find_package(SDL2 REQUIRED)
	find_package(glm REQUIRED)
	find_package(PNG REQUIRED)

endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

set(COMMON
        Connection.cpp
        Game.cpp)

set(CLIENT_FILES
        load_save_png.cpp
        main.cpp
        data_path.cpp
        compile_program.cpp
        vertex_color_program.cpp
        Scene.cpp
        Mode.cpp
        GameMode.cpp
        MenuMode.cpp
        Load.cpp
        MeshBuffer.cpp
        draw_text.cpp
        Sound.cpp)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(CLIENT_FILES ${CLIENT_FILES} gl_shims.cpp)
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

set(SERVER_FILES
        server.cpp)

add_custom_target(SDL2CopyBinaries
COMMAND ${CMAKE_COMMAND} -E copy ${KIT_DIR}/out/dist/SDL2.dll ${CMAKE_BINARY_DIR}/
)

add_custom_target(CopyAssets
COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/dist/menu.p ${CMAKE_BINARY_DIR}/
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/dist/indirect-pong.pnc ${CMAKE_BINARY_DIR}/
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/dist/indirect-pong.scene ${CMAKE_BINARY_DIR}/
)

add_executable(client ${CLIENT_FILES} ${COMMON})

target_include_directories(client PUBLIC ${SDL2_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR})

target_link_libraries(client ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} ${PNG_LIBRARIES})

add_executable(server ${SERVER_FILES} ${COMMON})

target_include_directories(server PUBLIC ${SDL2_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR})

target_link_libraries(server ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} ${PNG_LIBRARIES})

add_dependencies(client CopyAssets)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_dependencies(server SDL2CopyBinaries)
	add_dependencies(client SDL2CopyBinaries)
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")