# KTextEditor interface headers
ecm_generate_headers(KTextEditor_CamelCase_HEADERS
  HEADER_NAMES
  AnnotationInterface CodeCompletionModelControllerInterface HighlightInterface MovingCursor Range TextHintInterface
  Cursor MarkInterface MovingInterface RecoveryInterface
  Document  MovingRange SearchInterface View
  Attribute CommandInterface DocumentCursor MessageInterface MovingRangeFeedback SessionConfigInterface
  CodeCompletionInterface ConfigInterface Editor TemplateInterface
  CodeCompletionModel ConfigPage ConfigPageInterface ModificationInterface TemplateInterface2
  Application MainWindow Plugin
  PREFIX KTextEditor RELATIVE ktexteditor REQUIRED_HEADERS KTEXTEDITOR_PUBLIC_HEADERS)

# export headers to parent scope
set (KTEXTEDITOR_PUBLIC_HEADERS ${KTEXTEDITOR_PUBLIC_HEADERS} PARENT_SCOPE)

# install the public header files
install (FILES ${KTEXTEDITOR_PUBLIC_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/KTextEditor/ktexteditor COMPONENT Devel)

# install CamelCase headers
install(FILES ${KTextEditor_CamelCase_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/KTextEditor/KTextEditor COMPONENT Devel)
