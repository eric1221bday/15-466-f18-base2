cmake_minimum_required(VERSION 3.2)

project(walking-simulator)

set(CMAKE_CXX_STANDARD 14)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
find_package(PNG REQUIRED)

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

set(SERVER_FILES
        server.cpp)

add_executable(client ${CLIENT_FILES} ${COMMON})

target_include_directories(client PUBLIC ${OPENGL_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS})

target_link_libraries(client ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} PNG::PNG)

add_executable(server ${SERVER_FILES} ${COMMON})

target_include_directories(server PUBLIC ${OPENGL_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS})

target_link_libraries(server ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} PNG::PNG)