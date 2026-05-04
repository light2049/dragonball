cmake_minimum_required(VERSION 3.16)
project(DragonBall LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_DIR "D:/SFML-3.0.2/lib/cmake/SFML")
find_package(SFML 3.0.2 COMPONENTS Window Graphics CONFIG REQUIRED)

add_executable(DragonBall
        main.cpp
        src/Core/Game.cpp
        src/Core/InputManager.cpp
        src/Core/ResourceManager.cpp
        src/Characters/Fighter.cpp
        src/Characters/AnimationPlayer.cpp
        src/Utils/AirParser.cpp
        src/Utils/CnsParser.cpp
        src/UI/HUD.cpp
        src/Characters/Dummy.cpp
        src/Utils/CNSController.cpp
        src/Utils/CmdParser.cpp
        src/Core/StateRegistry.cpp
)

target_include_directories(DragonBall PRIVATE include)
target_link_libraries(DragonBall PRIVATE SFML::Window SFML::Graphics)

# ... (资源复制部分保持不变) ...
if(WIN32)
    add_custom_command(TARGET DragonBall POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "D:/SFML-3.0.2/bin" $<TARGET_FILE_DIR:DragonBall>
    )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/Data")
    add_custom_command(TARGET DragonBall POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/Data" "$<TARGET_FILE_DIR:DragonBall>/Data"
    )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET DragonBall POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:DragonBall>/assets"
    )
endif()