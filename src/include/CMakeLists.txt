# KTextEditor interface headers
ecm_generate_headers(AnnotationInterface CodeCompletionModelControllerInterface HighlightInterface MovingCursor Range TextHintInterface
  Cursor MarkInterface MovingInterface RecoveryInterface VariableInterface
  Document  MovingRange SearchInterface View
  Attribute CommandInterface DocumentCursor MessageInterface MovingRangeFeedback SessionConfigInterface
  CodeCompletionInterface ConfigInterface Editor ModeInterface  TemplateInterface
  CodeCompletionModel ConfigPage ConfigPageInterface ModificationInterface TemplateInterface2
  Application MainWindow ApplicationPlugin
  MODULE_NAME "KTextEditor" PREFIX "KTextEditor" RELATIVE "ktexteditor" REQUIRED_HEADERS KTEXTEDITOR_PUBLIC_HEADERS)
    
# export headers to parent scope
set (KTEXTEDITOR_PUBLIC_HEADERS ${KTEXTEDITOR_PUBLIC_HEADERS} PARENT_SCOPE)

# install the public header files
install (FILES ${KTEXTEDITOR_PUBLIC_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/KTextEditor/ktexteditor COMPONENT Devel)
   
# missing files ATM:   
# AnnotationModel Command AnnotationViewInterface CommandExtension Message ParameterizedSessionConfigInterface

# install CamelCase headers
install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/KTextEditor/KTextEditor DESTINATION ${INCLUDE_INSTALL_DIR}/KTextEditor COMPONENT Devel)
