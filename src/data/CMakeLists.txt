
########### install files ###############


# install the export file
install( FILES ktexteditor.desktop ktexteditorplugin.desktop DESTINATION  ${KDE_INSTALL_KSERVICETYPES5DIR} )

install( FILES katepartui.rc DESTINATION  ${KDE_INSTALL_KXMLGUI5DIR}/katepart RENAME katepart5ui.rc )

install( FILES katemoderc kateschemarc katesyntaxhighlightingrc DESTINATION  ${KDE_INSTALL_CONFDIR} )
