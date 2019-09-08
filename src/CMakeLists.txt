add_executable (roam roam_linux.c)
set_property(TARGET roam PROPERTY C_STANDARD 99)
target_link_libraries (roam ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${SDL2_LIBRARIES})
target_link_libraries (roam m)

