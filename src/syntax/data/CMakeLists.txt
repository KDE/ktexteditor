find_package(Perl REQUIRED)

add_custom_target(GeneratePhpXmlFiles ALL DEPENDS
 ${CMAKE_CURRENT_BINARY_DIR}/css-php.xml
 ${CMAKE_CURRENT_BINARY_DIR}/html-php.xml
 ${CMAKE_CURRENT_BINARY_DIR}/javascript-php.xml )

macro(GENERATE_PHP_XML targetFile srcFile)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${targetFile}
  COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate-php.pl < ${CMAKE_CURRENT_SOURCE_DIR}/${srcFile} > ${CMAKE_CURRENT_BINARY_DIR}/${targetFile}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generate-php.pl ${CMAKE_CURRENT_SOURCE_DIR}/${srcFile} )
endmacro(GENERATE_PHP_XML)

generate_php_xml(javascript-php.xml javascript.xml)
generate_php_xml(css-php.xml css.xml)
generate_php_xml(html-php.xml html.xml)

# all hls
file(GLOB HIGHLIGHTERS "${CMAKE_CURRENT_SOURCE_DIR}/*.xml")
set(ALL_HIGHLIGHTERS
    ${HIGHLIGHTERS}
    ${CMAKE_CURRENT_BINARY_DIR}/html-php.xml
    ${CMAKE_CURRENT_BINARY_DIR}/css-php.xml
    ${CMAKE_CURRENT_BINARY_DIR}/javascript-php.xml
)

# generate the resource file
set(INDEXFILE_IN ${CMAKE_CURRENT_BINARY_DIR}/index.qrc)
file(REMOVE ${INDEXFILE_IN})
file(APPEND ${INDEXFILE_IN} "<!DOCTYPE RCC>\n")
file(APPEND ${INDEXFILE_IN} "<RCC version=\"1.0\">\n")
file(APPEND ${INDEXFILE_IN} "<qresource prefix=\"/ktexteditor/syntax\">\n")
file(APPEND ${INDEXFILE_IN} "<file alias=\"language.xsd\">${CMAKE_CURRENT_SOURCE_DIR}/language.xsd</file>\n")
file(APPEND ${INDEXFILE_IN} "<file alias=\"index.json\">${CMAKE_CURRENT_BINARY_DIR}/index.json</file>\n")
foreach(highlighter ${ALL_HIGHLIGHTERS})
    get_filename_component(highlighter_base ${highlighter} NAME)
    file(APPEND ${INDEXFILE_IN} "<file alias=\"${highlighter_base}\">${highlighter}</file>\n")
endforeach()
file(APPEND ${INDEXFILE_IN} "</qresource>\n")
file(APPEND ${INDEXFILE_IN} "</RCC>\n")

# install only README & language XSD, the other stuff is in a resource
install(FILES README.md  language.xsd DESTINATION ${KDE_INSTALL_DATADIR}/katepart5/syntax)
