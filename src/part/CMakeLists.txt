# kate part itself just is core + the factory
add_library (katepart MODULE katepart.cpp)

configure_file(katepart.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/katepart.desktop @ONLY)

# service => json and install
kcoreaddons_desktop_to_json (katepart ${CMAKE_CURRENT_BINARY_DIR}/katepart.desktop SERVICE_TYPES kpart.desktop)
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/katepart.desktop  DESTINATION  ${KDE_INSTALL_KSERVICESDIR} )

# link the part, use kate part core + kde stuff
target_link_libraries (katepart KF5TextEditor KF5::SyntaxHighlighting)

# install the part
install (TARGETS katepart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/parts)
