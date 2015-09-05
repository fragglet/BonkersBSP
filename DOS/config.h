/* config.h for Watcom C/C++ v10 (probably also good for other DOS compilers)  */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you have the vprintf function.  */
/* #undef HAVE_VPRINTF */

/* Define as __inline if that's what the C compiler calls it.  */
#define inline

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#undef STDC_HEADERS

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
#undef WORDS_BIGENDIAN

/* Define if you have the isatty function.  */
#undef HAVE_ISATTY

/* Define if you have the tmpfile function.  */
#define HAVE_TMPFILE

/* Define if you have the unlink function.  */
#define HAVE_UNLINK

/* Define if you have the <unistd.h> header file.  */
#undef HAVE_UNISTD_H

/* Define if you have the <io.h> header file.  */
#define HAVE_IO_H

/* Define if you have the m library (-lm).  */
#undef HAVE_LIBM

/* Name of package */
#define PACKAGE "bsp"

/* Version number of package */
#define VERSION "5.1"

