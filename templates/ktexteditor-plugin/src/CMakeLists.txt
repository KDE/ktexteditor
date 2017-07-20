add_definitions(-DTRANSLATION_DOMAIN=\"%{APPNAMELC}\")

set(%{APPNAMELC}_SRCS
    %{APPNAMELC}plugin.cpp
    %{APPNAMELC}view.cpp
)

add_library(%{APPNAMELC} MODULE ${%{APPNAMELC}_SRCS})

target_link_libraries(%{APPNAMELC}
    KF5::TextEditor
    KF5::I18n
)

install(TARGETS %{APPNAMELC} DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)
