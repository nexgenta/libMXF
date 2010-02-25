AC_INIT([libMXF],[20100225])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_SYS_LARGEFILE

AC_CHECK_LIB([uuid], [uuid_generate])
AC_CHECK_LIB([m], [cos])

INCLUDES="$INCLUDES -I\${top_srcdir}/lib/include"
AC_SUBST([INCLUDES])

AC_CONFIG_FILES([
Makefile
lib/Makefile
test/Makefile
test/MXFDump/Makefile
test/test_lib/Makefile
test/test_utils/Makefile
examples/Makefile
examples/writeavidmxf/Makefile
examples/writeaviddv50/Makefile
examples/transfertop2/Makefile
examples/archive/Makefile
examples/archive/write/Makefile
examples/archive/info/Makefile
examples/archive/test/Makefile
examples/reader/Makefile
examples/avidmxfinfo/Makefile
tools/Makefile
tools/extract_avid_extensions/Makefile
])

AC_OUTPUT
