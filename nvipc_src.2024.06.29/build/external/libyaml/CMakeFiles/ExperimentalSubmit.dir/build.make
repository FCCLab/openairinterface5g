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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/vantuan_ngo/openairinterface5g/nvipc_src.2024.06.29

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/vantuan_ngo/openairinterface5g/nvipc_src.2024.06.29/build

# Utility rule file for ExperimentalSubmit.

# Include any custom commands dependencies for this target.
include external/libyaml/CMakeFiles/ExperimentalSubmit.dir/compiler_depend.make

# Include the progress variables for this target.
include external/libyaml/CMakeFiles/ExperimentalSubmit.dir/progress.make

external/libyaml/CMakeFiles/ExperimentalSubmit:
	cd /home/vantuan_ngo/openairinterface5g/nvipc_src.2024.06.29/build/external/libyaml && /usr/bin/ctest -D ExperimentalSubmit

ExperimentalSubmit: external/libyaml/CMakeFiles/ExperimentalSubmit
ExperimentalSubmit: external/libyaml/CMakeFiles/ExperimentalSubmit.dir/build.make
.PHONY : ExperimentalSubmit

# Rule to build all files generated by this target.
external/libyaml/CMakeFiles/ExperimentalSubmit.dir/build: ExperimentalSubmit
.PHONY : external/libyaml/CMakeFiles/ExperimentalSubmit.dir/build

external/libyaml/CMakeFiles/ExperimentalSubmit.dir/clean:
	cd /home/vantuan_ngo/openairinterface5g/nvipc_src.2024.06.29/build/external/libyaml && $(CMAKE_COMMAND) -P CMakeFiles/ExperimentalSubmit.dir/cmake_clean.cmake
.PHONY : external/libyaml/CMakeFiles/ExperimentalSubmit.dir/clean

external/libyaml/CMakeFiles/ExperimentalSubmit.dir/depend:
	cd /home/vantuan_ngo/openairinterface5g/nvipc_src.2024.06.29/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/vantuan_ngo/openairinterface5g/nvipc_src.2024.06.29 /home/vantuan_ngo/openairinterface5g/nvipc_src.2024.06.29/external/libyaml /home/vantuan_ngo/openairinterface5g/nvipc_src.2024.06.29/build /home/vantuan_ngo/openairinterface5g/nvipc_src.2024.06.29/build/external/libyaml /home/vantuan_ngo/openairinterface5g/nvipc_src.2024.06.29/build/external/libyaml/CMakeFiles/ExperimentalSubmit.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : external/libyaml/CMakeFiles/ExperimentalSubmit.dir/depend

