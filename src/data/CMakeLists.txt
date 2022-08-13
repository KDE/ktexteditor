# install the needed types for KTextEditor/Document & Plugin, we don't do this in KF6
if(QT_MAJOR_VERSION STREQUAL "5")
    install(FILES ktexteditor.desktop ktexteditorplugin.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPESDIR})
endif()
