# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/mir/workspace/flexric/src/sm/tc_sm

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/mir/workspace/flexric/src/sm/tc_sm/build

# Include any dependencies generated for this target.
include CMakeFiles/tc_sm.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/tc_sm.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/tc_sm.dir/flags.make

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/sm/sm_proc_data.c.o: CMakeFiles/tc_sm.dir/flags.make
CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/sm/sm_proc_data.c.o: /home/mir/workspace/flexric/src/sm/sm_proc_data.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/sm/sm_proc_data.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/sm/sm_proc_data.c.o   -c /home/mir/workspace/flexric/src/sm/sm_proc_data.c

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/sm/sm_proc_data.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/sm/sm_proc_data.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mir/workspace/flexric/src/sm/sm_proc_data.c > CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/sm/sm_proc_data.c.i

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/sm/sm_proc_data.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/sm/sm_proc_data.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mir/workspace/flexric/src/sm/sm_proc_data.c -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/sm/sm_proc_data.c.s

CMakeFiles/tc_sm.dir/tc_sm_agent.c.o: CMakeFiles/tc_sm.dir/flags.make
CMakeFiles/tc_sm.dir/tc_sm_agent.c.o: ../tc_sm_agent.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/tc_sm.dir/tc_sm_agent.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tc_sm.dir/tc_sm_agent.c.o   -c /home/mir/workspace/flexric/src/sm/tc_sm/tc_sm_agent.c

CMakeFiles/tc_sm.dir/tc_sm_agent.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tc_sm.dir/tc_sm_agent.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mir/workspace/flexric/src/sm/tc_sm/tc_sm_agent.c > CMakeFiles/tc_sm.dir/tc_sm_agent.c.i

CMakeFiles/tc_sm.dir/tc_sm_agent.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tc_sm.dir/tc_sm_agent.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mir/workspace/flexric/src/sm/tc_sm/tc_sm_agent.c -o CMakeFiles/tc_sm.dir/tc_sm_agent.c.s

CMakeFiles/tc_sm.dir/tc_sm_ric.c.o: CMakeFiles/tc_sm.dir/flags.make
CMakeFiles/tc_sm.dir/tc_sm_ric.c.o: ../tc_sm_ric.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/tc_sm.dir/tc_sm_ric.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tc_sm.dir/tc_sm_ric.c.o   -c /home/mir/workspace/flexric/src/sm/tc_sm/tc_sm_ric.c

CMakeFiles/tc_sm.dir/tc_sm_ric.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tc_sm.dir/tc_sm_ric.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mir/workspace/flexric/src/sm/tc_sm/tc_sm_ric.c > CMakeFiles/tc_sm.dir/tc_sm_ric.c.i

CMakeFiles/tc_sm.dir/tc_sm_ric.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tc_sm.dir/tc_sm_ric.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mir/workspace/flexric/src/sm/tc_sm/tc_sm_ric.c -o CMakeFiles/tc_sm.dir/tc_sm_ric.c.s

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/byte_array.c.o: CMakeFiles/tc_sm.dir/flags.make
CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/byte_array.c.o: /home/mir/workspace/flexric/src/util/byte_array.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/byte_array.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/byte_array.c.o   -c /home/mir/workspace/flexric/src/util/byte_array.c

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/byte_array.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/byte_array.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mir/workspace/flexric/src/util/byte_array.c > CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/byte_array.c.i

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/byte_array.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/byte_array.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mir/workspace/flexric/src/util/byte_array.c -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/byte_array.c.s

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c.o: CMakeFiles/tc_sm.dir/flags.make
CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c.o: /home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c.o   -c /home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c > CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c.i

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c.s

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c.o: CMakeFiles/tc_sm.dir/flags.make
CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c.o: /home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c.o   -c /home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c > CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c.i

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c.s

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c.o: CMakeFiles/tc_sm.dir/flags.make
CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c.o: /home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c.o   -c /home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c > CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c.i

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c.s

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c.o: CMakeFiles/tc_sm.dir/flags.make
CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c.o: /home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c.o   -c /home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c > CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c.i

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c.s

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c.o: CMakeFiles/tc_sm.dir/flags.make
CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c.o: /home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building C object CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c.o   -c /home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c > CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c.i

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c.s

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c.o: CMakeFiles/tc_sm.dir/flags.make
CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c.o: /home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building C object CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c.o   -c /home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c > CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c.i

CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c -o CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c.s

CMakeFiles/tc_sm.dir/enc/tc_enc_plain.c.o: CMakeFiles/tc_sm.dir/flags.make
CMakeFiles/tc_sm.dir/enc/tc_enc_plain.c.o: ../enc/tc_enc_plain.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building C object CMakeFiles/tc_sm.dir/enc/tc_enc_plain.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tc_sm.dir/enc/tc_enc_plain.c.o   -c /home/mir/workspace/flexric/src/sm/tc_sm/enc/tc_enc_plain.c

CMakeFiles/tc_sm.dir/enc/tc_enc_plain.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tc_sm.dir/enc/tc_enc_plain.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mir/workspace/flexric/src/sm/tc_sm/enc/tc_enc_plain.c > CMakeFiles/tc_sm.dir/enc/tc_enc_plain.c.i

CMakeFiles/tc_sm.dir/enc/tc_enc_plain.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tc_sm.dir/enc/tc_enc_plain.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mir/workspace/flexric/src/sm/tc_sm/enc/tc_enc_plain.c -o CMakeFiles/tc_sm.dir/enc/tc_enc_plain.c.s

CMakeFiles/tc_sm.dir/dec/tc_dec_plain.c.o: CMakeFiles/tc_sm.dir/flags.make
CMakeFiles/tc_sm.dir/dec/tc_dec_plain.c.o: ../dec/tc_dec_plain.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building C object CMakeFiles/tc_sm.dir/dec/tc_dec_plain.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tc_sm.dir/dec/tc_dec_plain.c.o   -c /home/mir/workspace/flexric/src/sm/tc_sm/dec/tc_dec_plain.c

CMakeFiles/tc_sm.dir/dec/tc_dec_plain.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tc_sm.dir/dec/tc_dec_plain.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mir/workspace/flexric/src/sm/tc_sm/dec/tc_dec_plain.c > CMakeFiles/tc_sm.dir/dec/tc_dec_plain.c.i

CMakeFiles/tc_sm.dir/dec/tc_dec_plain.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tc_sm.dir/dec/tc_dec_plain.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mir/workspace/flexric/src/sm/tc_sm/dec/tc_dec_plain.c -o CMakeFiles/tc_sm.dir/dec/tc_dec_plain.c.s

CMakeFiles/tc_sm.dir/ie/tc_data_ie.c.o: CMakeFiles/tc_sm.dir/flags.make
CMakeFiles/tc_sm.dir/ie/tc_data_ie.c.o: ../ie/tc_data_ie.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Building C object CMakeFiles/tc_sm.dir/ie/tc_data_ie.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/tc_sm.dir/ie/tc_data_ie.c.o   -c /home/mir/workspace/flexric/src/sm/tc_sm/ie/tc_data_ie.c

CMakeFiles/tc_sm.dir/ie/tc_data_ie.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/tc_sm.dir/ie/tc_data_ie.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/mir/workspace/flexric/src/sm/tc_sm/ie/tc_data_ie.c > CMakeFiles/tc_sm.dir/ie/tc_data_ie.c.i

CMakeFiles/tc_sm.dir/ie/tc_data_ie.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/tc_sm.dir/ie/tc_data_ie.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/mir/workspace/flexric/src/sm/tc_sm/ie/tc_data_ie.c -o CMakeFiles/tc_sm.dir/ie/tc_data_ie.c.s

# Object files for target tc_sm
tc_sm_OBJECTS = \
"CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/sm/sm_proc_data.c.o" \
"CMakeFiles/tc_sm.dir/tc_sm_agent.c.o" \
"CMakeFiles/tc_sm.dir/tc_sm_ric.c.o" \
"CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/byte_array.c.o" \
"CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c.o" \
"CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c.o" \
"CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c.o" \
"CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c.o" \
"CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c.o" \
"CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c.o" \
"CMakeFiles/tc_sm.dir/enc/tc_enc_plain.c.o" \
"CMakeFiles/tc_sm.dir/dec/tc_dec_plain.c.o" \
"CMakeFiles/tc_sm.dir/ie/tc_data_ie.c.o"

# External object files for target tc_sm
tc_sm_EXTERNAL_OBJECTS =

libtc_sm.so: CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/sm/sm_proc_data.c.o
libtc_sm.so: CMakeFiles/tc_sm.dir/tc_sm_agent.c.o
libtc_sm.so: CMakeFiles/tc_sm.dir/tc_sm_ric.c.o
libtc_sm.so: CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/byte_array.c.o
libtc_sm.so: CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/defer.c.o
libtc_sm.so: CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/alg/eq_float.c.o
libtc_sm.so: CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_arr.c.o
libtc_sm.so: CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/seq_container/seq_ring.c.o
libtc_sm.so: CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/assoc_rb_tree.c.o
libtc_sm.so: CMakeFiles/tc_sm.dir/home/mir/workspace/flexric/src/util/alg_ds/ds/assoc_container/bimap.c.o
libtc_sm.so: CMakeFiles/tc_sm.dir/enc/tc_enc_plain.c.o
libtc_sm.so: CMakeFiles/tc_sm.dir/dec/tc_dec_plain.c.o
libtc_sm.so: CMakeFiles/tc_sm.dir/ie/tc_data_ie.c.o
libtc_sm.so: CMakeFiles/tc_sm.dir/build.make
libtc_sm.so: CMakeFiles/tc_sm.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_14) "Linking C shared library libtc_sm.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/tc_sm.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/tc_sm.dir/build: libtc_sm.so

.PHONY : CMakeFiles/tc_sm.dir/build

CMakeFiles/tc_sm.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/tc_sm.dir/cmake_clean.cmake
.PHONY : CMakeFiles/tc_sm.dir/clean

CMakeFiles/tc_sm.dir/depend:
	cd /home/mir/workspace/flexric/src/sm/tc_sm/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/mir/workspace/flexric/src/sm/tc_sm /home/mir/workspace/flexric/src/sm/tc_sm /home/mir/workspace/flexric/src/sm/tc_sm/build /home/mir/workspace/flexric/src/sm/tc_sm/build /home/mir/workspace/flexric/src/sm/tc_sm/build/CMakeFiles/tc_sm.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/tc_sm.dir/depend
