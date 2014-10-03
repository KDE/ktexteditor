cmake_minimum_required(VERSION 2.8.12)

project(KTextEditor)

# ECM setup
find_package(ECM 1.3.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(FeatureSummary)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMPackageConfigHelpers)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings)
include(KDECMakeSettings)
include(GenerateExportHeader)

set(KF5_VERSION "5.3.0") # handled by release scripts
set(KF5_DEP_VERSION "5.3.0") # handled by release scripts

ecm_setup_version(
  ${KF5_VERSION}
  VARIABLE_PREFIX KTEXTEDITOR
  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/ktexteditor_version.h"
  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5TextEditorConfigVersion.cmake"
  SOVERSION 5
)

# Dependencies
set(REQUIRED_QT_VERSION "5.2.0")

# Required Qt5 components to build this framework
find_package(Qt5 ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Core Widgets Script PrintSupport Xml XmlPatterns)

find_package(KF5Archive ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5Config ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5GuiAddons ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5I18n ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5KIO ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5Parts ${KF5_DEP_VERSION} REQUIRED)
find_package(KF5Sonnet ${KF5_DEP_VERSION} REQUIRED)

# libgit2 integration
find_package(LibGit2 "0.20.0")
if(LIBGIT2_FOUND)
    add_definitions(-DHAVE_GIT2)
    set (KTEXTEDITOR_OPTIONAL_LIBS ${KTEXTEDITOR_OPTIONAL_LIBS} LibGit2::LibGit2)
endif()

# vi mode on per default
option (BUILD_VIMODE "Build vimode in" ON)

# Subdirectories
add_definitions(-DTRANSLATION_DOMAIN=\"ktexteditor5\")
if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/po")
    ki18n_install(po)
endif()
add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()

# Create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KF5TextEditor")

ecm_configure_package_config_file(
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
  DESTINATION "${KF5_INCLUDE_INSTALL_DIR}"
  COMPONENT Devel
)

# config.h
check_symbol_exists (fdatasync unistd.h HAVE_FDATASYNC)
configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# let our config.h be found first in any case
include_directories (BEFORE ${CMAKE_CURRENT_BINARY_DIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
