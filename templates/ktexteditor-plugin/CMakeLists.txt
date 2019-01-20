cmake_minimum_required(VERSION 3.0)

project(%{APPNAMELC})

set(REQUIRED_ECM_VERSION "5.23.0")
find_package(ECM ${REQUIRED_ECM_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

set(REQUIRED_KF5_VERSION "5.23.0")
find_package(KF5 ${REQUIRED_KF5_VERSION} REQUIRED COMPONENTS
    I18n
    TextEditor
)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
