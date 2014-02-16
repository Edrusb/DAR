LIBDAR_MAJOR = `grep LIBDAR_COMPILE_TIME_MAJOR ../src/libdar/libdar.hpp | cut -d '=' -f 2 | cut -d ';' -f 1 | cut -d ' ' -f 2`
LIBDAR_MINOR = `grep LIBDAR_COMPILE_TIME_MINOR ../src/libdar/libdar.hpp | cut -d '=' -f 2 | cut -d ';' -f 1 | cut -d ' ' -f 2`

dist_noinst_DATA = dar.gif make_cygwin_dist make_rpm batch_cygwin batch_linux batch_linux_ea README batch_solaris
noinst_PROGRAMS=todos
todos_SOURCES=todos.c

