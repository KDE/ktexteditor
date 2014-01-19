
########### install files ###############


# install the export file
install( FILES ktexteditor.desktop ktexteditorplugin.desktop DESTINATION  ${SERVICETYPES_INSTALL_DIR} )

install( FILES katepartui.rc DESTINATION  ${DATA_INSTALL_DIR}/katepart )

install( FILES katemoderc kateschemarc katesyntaxhighlightingrc DESTINATION  ${CONFIG_INSTALL_DIR} )
