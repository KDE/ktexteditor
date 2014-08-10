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
set (ALL_HIGHLIGHTERS ${HIGHLIGHTERS} ${CMAKE_CURRENT_BINARY_DIR}/html-php.xml ${CMAKE_CURRENT_BINARY_DIR}/css-php.xml ${CMAKE_CURRENT_BINARY_DIR}/javascript-php.xml)

# our xsd
set (LANGUAGE_XSD "${CMAKE_CURRENT_SOURCE_DIR}/language.xsd")

# tool to create json index of highlightings, will validate all highlightings, too!
add_executable(katehighlightingindexer katehighlightingindexer.cpp)
target_link_libraries(katehighlightingindexer Qt5::XmlPatterns
)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/index.json
  COMMAND katehighlightingindexer ${CMAKE_CURRENT_BINARY_DIR}/index.json ${LANGUAGE_XSD} ${ALL_HIGHLIGHTERS}
  DEPENDS katehighlightingindexer.cpp ${LANGUAGE_XSD} ${ALL_HIGHLIGHTERS})
add_custom_target(GenerateIndexFile ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/index.json)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/index.json ${LANGUAGE_XSD} ${ALL_HIGHLIGHTERS} DESTINATION ${DATA_INSTALL_DIR}/katepart5/syntax)

endif()
