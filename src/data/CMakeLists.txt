
########### install files ###############


# install the export file
install( FILES ktexteditor.desktop ktexteditorplugin.desktop DESTINATION  ${SERVICETYPES_INSTALL_DIR} )

install( FILES katepartui.rc DESTINATION  ${KXMLGUI_INSTALL_DIR}/katepart RENAME katepart5ui.rc )

install( FILES katemoderc kateschemarc katesyntaxhighlightingrc DESTINATION  ${CONFIG_INSTALL_DIR} )
