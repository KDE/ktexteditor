# kate part itself just is core + the factory
add_library (katepart MODULE katepart.cpp)

# no KTextEditor/Document in KF6
if(QT_MAJOR_VERSION STREQUAL "5")
    set(KF_EXTRA_PART_TYPES ",KTextEditor/Document")
else()
    set(KF_EXTRA_PART_TYPES "")
endif()

configure_file(katepart.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/katepart.desktop @ONLY)

# service => json and install
kcoreaddons_desktop_to_json (katepart ${CMAKE_CURRENT_BINARY_DIR}/katepart.desktop SERVICE_TYPES kpart.desktop)

if (QT_MAJOR_VERSION STREQUAL "5")
    install( FILES ${CMAKE_CURRENT_BINARY_DIR}/katepart.desktop  DESTINATION  ${KDE_INSTALL_KSERVICESDIR} )
endif()

# link the part, use kate part core + kde stuff
target_link_libraries (katepart KF5TextEditor KF5::SyntaxHighlighting)

# install the part
install (TARGETS katepart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/parts)
