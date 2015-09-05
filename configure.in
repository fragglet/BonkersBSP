dnl Process this file with autoconf to produce a configure script.
AC_INIT([bsp],[5.2])

dnl --- Fire up automake
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip check-news -Woverride -Wobsolete -Wportability -Wsyntax -Wunsupported])
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

dnl C compiler options
AC_DEFUN([AC_C_COMPILE_FLAGS],[
NEW_CFLAGS="$CFLAGS"
for ac_flag in $1
do
 AC_MSG_CHECKING(whether compiler supports $ac_flag)
 CFLAGS="$NEW_CFLAGS $ac_flag"
 AC_TRY_COMPILE(,[
  void f() {};
 ],[
  NEW_CFLAGS="$CFLAGS"
  AC_MSG_RESULT(yes)
 ],AC_MSG_RESULT(no))
done
CFLAGS="$NEW_CFLAGS"
])

CFLAGS="-O2 -fomit-frame-pointer"
AC_C_COMPILE_FLAGS([-Wall -Wextra -ffast-math -pipe -march=native -Winline -Wwrite-strings -Wundef -Wbad-function-cast -Wcast-align -Wcast-qual -Wdeclaration-after-statement])
AC_ARG_ENABLE(debug,AC_HELP_STRING([--enable-debug],[enables debugging with gdb]),
CFLAGS="-g"
AC_C_COMPILE_FLAGS([-Wmissing-prototypes -Waggregate-return -Wmissing-noreturn -Wredundant-decls -Wnested-externs -Wold-style-definition -Wformat -Wmissing-format-attribute -Wvariadic-macros -Wdisabled-optimization -Wlarger-than-128000 -Wshadow])
)
AC_ARG_ENABLE(profile,AC_HELP_STRING([--enable-profile],[turns on profiling]),
CFLAGS="-pg"
)

dnl Checks for programs.
AC_PROG_CC
AC_C_INLINE
AC_C_BIGENDIAN
AC_PROG_MAKE_SET

dnl Checks for libraries.
AC_CHECK_LIB(m, sqrt)

dnl Debugging
AM_WITH_DMALLOC

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(isatty unlink tmpfile)

AC_OUTPUT(Makefile test-wads/Makefile DOS/Makefile Win32/Makefile)
