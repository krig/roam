add_executable (roam roam_linux.c)
set_property(TARGET roam PROPERTY C_STANDARD 11)
target_link_libraries (roam ${OPENGL_LIBRARIES})
target_link_libraries (roam ${GLEW_LIBRARIES})
target_link_libraries (roam ${SDL2_LIBRARIES})
target_link_libraries (roam m)

