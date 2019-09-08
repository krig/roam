add_executable (roam roam_linux.c)
set_property(TARGET roam PROPERTY C_STANDARD 11)
target_link_libraries(roam GLEW::GLEW)
target_include_directories(roam PUBLIC
    "../stb"
    ${SDL2_INCLUDE_DIRS})
target_link_libraries(roam
    ${SDL2_LIBRARIES}
    m)

