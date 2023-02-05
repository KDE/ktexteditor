cmake_minimum_required(VERSION 3.16)

set(KF_VERSION "5.103.0") # handled by release scripts
set(KF_DEP_VERSION "5.103.0") # handled by release scripts
project(KTextEditor VERSION ${KF_VERSION})

# ECM setup
include(FeatureSummary)
find_package(ECM 5.103.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://commits.kde.org/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

# add own modules to search path, too
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDEGitCommitHooks)

include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMGenerateExportHeader)
include(CMakePackageConfigHelpers)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(KDEPackageAppTemplates)
include(GenerateExportHeader)
include(ECMAddQch)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)

option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")

option(ENABLE_PCH "Enable Precompiled Headers support" OFF)

if (NOT WIN32)
    set(ENABLE_KAUTH_DEFAULT ON)
else()
    set(ENABLE_KAUTH_DEFAULT OFF)
endif()
option(ENABLE_KAUTH "Enable KAuth integration" ${ENABLE_KAUTH_DEFAULT})
add_feature_info(KAUTH ${ENABLE_KAUTH} "KAuth integration for editing system files")

ecm_setup_version(
  PROJECT
  VARIABLE_PREFIX KTEXTEDITOR
  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/ktexteditor_version.h"
  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5TextEditorConfigVersion.cmake"
  SOVERSION 5
)

# Dependencies
set(REQUIRED_QT_VERSION 5.15.2)

# Required Qt components to build this framework
find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Core Widgets Qml
  PrintSupport Xml)
if (QT_MAJOR_VERSION STREQUAL "6")
    find_package(Qt6Core5Compat)
endif()

find_package(KF5Archive ${KF_DEP_VERSION} REQUIRED)
find_package(KF5Config ${KF_DEP_VERSION} REQUIRED)
find_package(KF5GuiAddons ${KF_DEP_VERSION} REQUIRED)
find_package(KF5I18n ${KF_DEP_VERSION} REQUIRED)
find_package(KF5KIO ${KF_DEP_VERSION} REQUIRED)
find_package(KF5Parts ${KF_DEP_VERSION} REQUIRED)
find_package(KF5Sonnet ${KF_DEP_VERSION} REQUIRED)
find_package(KF5SyntaxHighlighting ${KF_DEP_VERSION} REQUIRED)
find_package(KF5WindowSystem ${KF_DEP_VERSION} REQUIRED)

if (ENABLE_KAUTH)
    find_package(KF5Auth ${KF_DEP_VERSION} REQUIRED)
    set(HAVE_KAUTH ${KF5Auth_FOUND})
else()
    set(HAVE_KAUTH FALSE)
endif()

# EditorConfig integration
find_package(EditorConfig)

# forbid some old things
set(EXCLUDE_DEPRECATED_BEFORE_AND_AT 0 CACHE STRING "Control the range of deprecated API excluded from the build [default=0].")
ecm_set_disabled_deprecation_versions(
    QT 5.15.2
    KF 5.97.0
    KWIDGETSADDONS 5.77 # We need KSelectAction::triggered(QString)
)

# Subdirectories
add_definitions(-DTRANSLATION_DOMAIN=\"ktexteditor5\")
ki18n_install(po)

add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
add_subdirectory(templates)

# Create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF5TextEditor")

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KF5TextEditor_QCH
        FILE KF5TextEditorQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KF5TextEditorQchTargets.cmake\")")
endif()

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KF5TextEditorConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5TextEditorConfig.cmake"
  INSTALL_DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KF5TextEditorConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5TextEditorConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT
  KF5TextEditorTargets
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  FILE KF5TextEditorTargets.cmake
  NAMESPACE KF5::
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/ktexteditor_version.h"
  DESTINATION "${KDE_INSTALL_INCLUDEDIR_KF}/KTextEditor"
  COMPONENT Devel
)

# config.h
check_symbol_exists (fdatasync unistd.h HAVE_FDATASYNC)
configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# let our config.h be found first in any case
include_directories (BEFORE ${CMAKE_CURRENT_BINARY_DIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
