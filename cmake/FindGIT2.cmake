#.rst:
# FindGIT2
# -------
#
# Try to find libgit2 on a Unix system.
#
# This will define the following variables:
#
# ``GIT2_FOUND``
#     True if (the requested version of) GIT2 is available
# ``GIT2_LIBRARIES``
#     This can be passed to target_link_libraries() instead of the ``GIT2::GIT2``
#     target
# ``GIT2_INCLUDE_DIRS``
#     This should be passed to target_include_directories() if the target is not
#     used for linking
# ``GIT2_DEFINITIONS``
#     This should be passed to target_compile_options() if the target is not
#     used for linking
#
# If ``GIT2_FOUND`` is TRUE, it will also define the following imported target:
#
# ``GIT2::GIT2``
#     The GIT2 library
#
# In general we recommend using the imported target, as it is easier to use.
# Bear in mind, however, that if the target is in the link interface of an
# exported library, it must be made available by the package config file.

#=============================================================================
# Copyright 2014 Alex Merry <alex.merry@kde.org>
# Copyright 2014 Martin Gräßlin <mgraesslin@kde.org>
# Copyright 2014 Christoph Cullmann <cullmann@kde.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

if(CMAKE_VERSION VERSION_LESS 2.8.12)
    message(FATAL_ERROR "CMake 2.8.12 is required by FindGIT2.cmake")
endif()
if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 2.8.12)
    message(AUTHOR_WARNING "Your project should require at least CMake 2.8.12 to use FindGIT2.cmake")
endif()

# Use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
find_package(PkgConfig)
pkg_check_modules(PKG_GIT2 QUIET git2)

set(GIT2_DEFINITIONS ${PKG_GIT2_CFLAGS_OTHER})

find_path(GIT2_INCLUDE_DIR
    NAMES
        git2.h
    HINTS
        ${PKG_GIT2_INCLUDE_DIRS}
    PATH_SUFFIXES
        GIT2
)
find_library(GIT2_LIBRARY
    NAMES
        git2
    HINTS
        ${PKG_GIT2_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GIT2
    FOUND_VAR
        GIT2_FOUND
    REQUIRED_VARS
        GIT2_LIBRARY
        GIT2_INCLUDE_DIR
)

if(GIT2_FOUND AND NOT TARGET GIT2::GIT2)
    add_library(GIT2::GIT2 UNKNOWN IMPORTED)
    set_target_properties(GIT2::GIT2 PROPERTIES
        IMPORTED_LOCATION "${GIT2_LIBRARY}"
        INTERFACE_COMPILE_OPTIONS "${GIT2_DEFINITIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${GIT2_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(GIT2_LIBRARY GIT2_INCLUDE_DIR)

# compatibility variables
set(GIT2_LIBRARIES ${GIT2_LIBRARY})
set(GIT2_INCLUDE_DIRS ${GIT2_INCLUDE_DIR})

include(FeatureSummary)
set_package_properties(GIT2 PROPERTIES
    URL "https://libgit2.github.com/"
    DESCRIPTION "A plain C library to interface with the git version control system."
)
