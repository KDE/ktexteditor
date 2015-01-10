# install default libraries files
install (DIRECTORY libraries/ DESTINATION ${KDE_INSTALL_DATADIR}/katepart5/script/libraries
         FILES_MATCHING PATTERN "*.js")

# install default commands files
install (DIRECTORY commands/ DESTINATION ${KDE_INSTALL_DATADIR}/katepart5/script/commands
         FILES_MATCHING PATTERN "*.js")

# install default indentation files
install (DIRECTORY indentation/ DESTINATION ${KDE_INSTALL_DATADIR}/katepart5/script/indentation
         FILES_MATCHING PATTERN "*.js")

# install default files files
# this is a place where scripts can store additional data
install (DIRECTORY files/ DESTINATION ${KDE_INSTALL_DATADIR}/katepart5/script/files)
