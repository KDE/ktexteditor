# install the desktop files
install(FILES ktexteditor.desktop ktexteditorplugin.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})

# install the config files
install(FILES katemoderc kateschemarc katesyntaxhighlightingrc DESTINATION ${KDE_INSTALL_CONFDIR})
