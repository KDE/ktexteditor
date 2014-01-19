# service => json and install
kservice_desktop_to_json (katepart.desktop)
install( FILES katepart.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

# kate part itself just is core + the factory
add_library (katepart MODULE katepart.cpp)

# link the part, use kate part core + kde stuff
target_link_libraries (katepart KF5TextEditor)

# install the part
install (TARGETS katepart DESTINATION ${PLUGIN_INSTALL_DIR})
