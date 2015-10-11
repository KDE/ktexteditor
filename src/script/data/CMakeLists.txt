# collect all files
file(GLOB_RECURSE ALL_FILES
    RELATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}
    "libraries/*.*"
    "commands/*.*"
    "indentation/*.*"
    "files/*.*")

# generate the resource file
set(INDEXFILE_IN ${CMAKE_CURRENT_BINARY_DIR}/script.qrc)
file(REMOVE ${INDEXFILE_IN})
file(APPEND ${INDEXFILE_IN} "<!DOCTYPE RCC>\n")
file(APPEND ${INDEXFILE_IN} "<RCC version=\"1.0\">\n")
file(APPEND ${INDEXFILE_IN} "<qresource prefix=\"/ktexteditor/script\">\n")
foreach(highlighter ${ALL_FILES})
    file(RELATIVE_PATH highlighter_base ${CMAKE_CURRENT_SOURCE_DIR} ${highlighter})
    file(APPEND ${INDEXFILE_IN} "<file alias=\"${highlighter_base}\">${highlighter}</file>\n")
endforeach()
file(APPEND ${INDEXFILE_IN} "</qresource>\n")
file(APPEND ${INDEXFILE_IN} "</RCC>\n")

# install only README
install(FILES README.md DESTINATION ${KDE_INSTALL_DATADIR}/katepart5/script)
