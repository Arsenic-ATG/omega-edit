# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /Users/runner/work/_temp/504157251/cmake-3.22.2-macos-universal/CMake.app/Contents/bin/cmake

# The command to remove a file.
RM = /Users/runner/work/_temp/504157251/cmake-3.22.2-macos-universal/CMake.app/Contents/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/runner/work/omega-edit/omega-edit

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/runner/work/omega-edit/omega-edit/build

# Utility rule file for sphinx.

# Include any custom commands dependencies for this target.
include CMakeFiles/sphinx.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/sphinx.dir/progress.make

CMakeFiles/sphinx: docs/sphinx/index.html

docs/sphinx/index.html: ../sphinx/index.rst
docs/sphinx/index.html: ../sphinx/conf.py
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/Users/runner/work/omega-edit/omega-edit/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating documentation with Sphinx"
	/usr/local/bin/sphinx-build -b html -Dbreathe_projects.omega_edit="/Users/runner/work/omega-edit/omega-edit/build/docs/xml" /Users/runner/work/omega-edit/omega-edit/sphinx /Users/runner/work/omega-edit/omega-edit/build/docs/sphinx

sphinx: CMakeFiles/sphinx
sphinx: docs/sphinx/index.html
sphinx: CMakeFiles/sphinx.dir/build.make
.PHONY : sphinx

# Rule to build all files generated by this target.
CMakeFiles/sphinx.dir/build: sphinx
.PHONY : CMakeFiles/sphinx.dir/build

CMakeFiles/sphinx.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/sphinx.dir/cmake_clean.cmake
.PHONY : CMakeFiles/sphinx.dir/clean

CMakeFiles/sphinx.dir/depend:
	cd /Users/runner/work/omega-edit/omega-edit/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/runner/work/omega-edit/omega-edit /Users/runner/work/omega-edit/omega-edit /Users/runner/work/omega-edit/omega-edit/build /Users/runner/work/omega-edit/omega-edit/build /Users/runner/work/omega-edit/omega-edit/build/CMakeFiles/sphinx.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/sphinx.dir/depend

