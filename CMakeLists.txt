cmake_minimum_required(VERSION 2.8.12)

project(KTextEditor)

# ECM setup
find_package(ECM 0.0.9 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(FeatureSummary)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(CMakePackageConfigHelpers)
include(CheckFunctionExists)
include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings)
include(KDECMakeSettings)

set(KF5_VERSION "5.0.0")

ecm_setup_version(
  ${KF5_VERSION}
  VARIABLE_PREFIX KTEXTEDITOR
  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/ktexteditor_version.h"
  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KTextEditorConfigVersion.cmake"
)

# Dependencies
set(REQUIRED_QT_VERSION "5.2.0")

# Required Qt5 components to build this framework
find_package(Qt5 ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Core Widgets Script)

# TODO: Add all required find_package() calls here

if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  # KF5 frameworks
  #
  # Content of this block is only executed when building the framework
  # standalone. KF5 frameworks required to build this framework should be added
  # here. It is done this way to avoid chicken-and-egg problem when building
  # KF5 as a whole: one cannot depend on packages which have not been installed
  # yet. For example:
  find_package(KF5Archive ${KF5_VERSION} REQUIRED)
  find_package(KF5Config ${KF5_VERSION} REQUIRED)
  find_package(KF5GuiAddons ${KF5_VERSION} REQUIRED)
  find_package(KF5I18n ${KF5_VERSION} REQUIRED)
  find_package(KF5Init ${KF5_VERSION} REQUIRED)
  find_package(KF5JobWidgets ${KF5_VERSION} REQUIRED)
  find_package(KF5KCMUtils ${KF5_VERSION} REQUIRED)
  find_package(KF5KIO ${KF5_VERSION} REQUIRED)
  find_package(KF5Parts ${KF5_VERSION} REQUIRED)
  find_package(KF5PrintUtils ${KF5_VERSION} REQUIRED)
  find_package(KF5Sonnet ${KF5_VERSION} REQUIRED)
  find_package(KF5XmlGui ${KF5_VERSION} REQUIRED)
endif()

# Subdirectories
add_subdirectory(src)
add_subdirectory(autotests)
#add_subdirectory(tests)

# Create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KTextEditor")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KTextEditorConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KTextEditorConfig.cmake"
  INSTALL_DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KTextEditorConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KTextEditorConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT
  KTextEditorTargets
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  FILE KTextEditorTargets.cmake
  NAMESPACE KF5::
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/ktexteditor_version.h"
  DESTINATION "${INCLUDE_INSTALL_DIR}"
  COMPONENT Devel
)

# config.h
if(NOT APPLE)
  check_function_exists (fdatasync HAVE_FDATASYNC)
  check_function_exists (ctermid HAVE_CTERMID)
endif(NOT APPLE)

configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()
