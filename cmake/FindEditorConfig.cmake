#.rest:
# FindEditorConfig
# --------------
#
# Try to find EditorConfig on this system.
#
# This will define the following variables:
#
# ``EditorConfig_FOUND``
#    True if inotify is available
# ``EditorConfig_LIBRARIES``
#    This has to be passed to target_link_libraries()
# ``EditorConfig_INCLUDE_DIRS``
#    This has to be passed to target_include_directories()

#=============================================================================
# Copyright 2017 Christoph Cullmann <cullmann@kde.org>
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

find_path(EditorConfig_INCLUDE_DIRS editorconfig/editorconfig.h)

if(EditorConfig_INCLUDE_DIRS)
    find_library(EditorConfig_LIBRARIES NAMES editorconfig)
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(EditorConfig
        FOUND_VAR
            EditorConfig_FOUND
        REQUIRED_VARS
            EditorConfig_LIBRARIES
            EditorConfig_INCLUDE_DIRS
    )
    mark_as_advanced(EditorConfig_LIBRARIES EditorConfig_INCLUDE_DIRS)
    include(FeatureSummary)
    set_package_properties(EditorConfig PROPERTIES
        URL "https://editorconfig.org/"
        DESCRIPTION "EditorConfig editor configuration file support."
    )
else()
   set(EditorConfig_FOUND FALSE)
endif()

mark_as_advanced(EditorConfig_LIBRARIES EditorConfig_INCLUDE_DIRS) 
