########### install files ###############

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

# allow to disable the compilation + install for update xml generation
if (NOT ONLY_GENERATE_XML)

# all hls
file(GLOB HIGHLIGHTERS "${CMAKE_CURRENT_SOURCE_DIR}/*.xml")
set(ALL_HIGHLIGHTERS
    ${HIGHLIGHTERS}
    ${CMAKE_CURRENT_BINARY_DIR}/html-php.xml
    ${CMAKE_CURRENT_BINARY_DIR}/css-php.xml
    ${CMAKE_CURRENT_BINARY_DIR}/javascript-php.xml
)

# we create a resource file and an index json
set(INDEXFILE_IN ${CMAKE_CURRENT_BINARY_DIR}/index.qrc)
set(INDEXFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/index.json)

# our xsd
set (LANGUAGE_XSD "${CMAKE_CURRENT_SOURCE_DIR}/language.xsd")

# write out file containing list of all highlighter files
file(REMOVE ${INDEXFILE_IN})
file(APPEND ${INDEXFILE_IN} "<!DOCTYPE RCC>\n")
file(APPEND ${INDEXFILE_IN} "<RCC version=\"1.0\">\n")
file(APPEND ${INDEXFILE_IN} "<qresource prefix=\"/ktexteditor/syntax\">\n")
file(APPEND ${INDEXFILE_IN} "<file alias=\"index.json\">${INDEXFILE_OUT}</file>\n")
foreach(highlighter ${ALL_HIGHLIGHTERS})
    get_filename_component(highlighter_base ${highlighter} NAME)
    file(APPEND ${INDEXFILE_IN} "<file alias=\"${highlighter_base}\">${highlighter}</file>\n")
endforeach()
file(APPEND ${INDEXFILE_IN} "</qresource>\n")
file(APPEND ${INDEXFILE_IN} "</RCC>\n")

# tool to create json index of highlightings, will validate all highlightings, too!
add_executable(katehighlightingindexer katehighlightingindexer.cpp)
target_link_libraries(katehighlightingindexer Qt5::XmlPatterns
)
add_custom_command(OUTPUT ${INDEXFILE_OUT}
  COMMAND katehighlightingindexer ${INDEXFILE_OUT} ${LANGUAGE_XSD} ${INDEXFILE_IN}
  DEPENDS katehighlightingindexer.cpp ${LANGUAGE_XSD} ${ALL_HIGHLIGHTERS} ${INDEXFILE_IN})
add_custom_target(GenerateIndexFile ALL DEPENDS ${INDEXFILE_OUT})

install(FILES ${INDEXFILE_OUT} ${LANGUAGE_XSD} ${ALL_HIGHLIGHTERS} DESTINATION ${KDE_INSTALL_DATADIR}/katepart5/syntax)

endif()
