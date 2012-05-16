LIBDAR_MAJOR = `grep LIBDAR_COMPILE_TIME_MAJOR ../src/libdar/libdar.hpp | cut -d '=' -f 2 | cut -d ';' -f 1 | cut -d ' ' -f 2`
LIBDAR_MINOR = `grep LIBDAR_COMPILE_TIME_MINOR ../src/libdar/libdar.hpp | cut -d '=' -f 2 | cut -d ';' -f 1 | cut -d ' ' -f 2`

%.spec : %.proto ../configure.ac
	sed -e "s%DAR_VERSION%$(PACKAGE_VERSION)%g" -e "s%GENERATE_COMMENT%THIS IS A GENERATED FILE DO NOT EDIT !%g" -e "s%LIBDAR_MAJOR%$(LIBDAR_MAJOR)%g" -e "s%LIBDAR_MINOR%$(LIBDAR_MINOR)%g" $< > $@

dar.rpm.spec : dar.rpm.proto
dar.rpm.spec : dar.rpm.proto
dar32.rpm.spec : dar32.rpm.proto
dar32_ea.rpm.spec : dar32_ea.rpm.proto
dar64.rpm.spec : dar64.rpm.proto
dar64_ea.rpm.spec : dar64_ea.rpm.proto
dar_ea.rpm.spec : dar_ea.rpm.proto

dist_noinst_DATA = dar.gif make_cygwin_dist make_rpm batch_cygwin batch_linux batch_linux_ea README batch_solaris dar.rpm.proto dar32.rpm.proto dar32_ea.rpm.proto dar64.rpm.proto dar64_ea.rpm.proto dar_ea.rpm.proto

noinst_PROGRAMS=todos specs

specs$(EXEEXT) : dar.rpm.spec dar32.rpm.spec dar32_ea.rpm.spec dar64.rpm.spec dar64_ea.rpm.spec dar_ea.rpm.spec
	@echo "RPM spec files generated"
clean_specs :
	rm -f dar.rpm.spec dar32.rpm.spec dar32_ea.rpm.spec dar64.rpm.spec dar64_ea.rpm.spec dar_ea.rpm.spec

todos_SOURCES=todos.c
specs_SOURCES=dar.rpm.spec dar32.rpm.spec dar32_ea.rpm.spec dar64.rpm.spec dar64_ea.rpm.spec dar_ea.rpm.spec

