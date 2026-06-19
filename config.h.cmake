#cmakedefine SIZEOF_UNSIGNED_LONG @SIZEOF_UNSIGNED_LONG@

#if !defined( HAVE_STDLIB_H )
#cmakedefine HAVE_STDLIB_H
#endif // HAVE_STDLIB_H

#if !defined( HAVE_LONG_DOUBLE )
#cmakedefine HAVE_LONG_DOUBLE
#endif // HAVE_LONG_DOUBLE

#cmakedefine HAVE_L_FUNCS
#cmakedefine HAVE_IEEEFP_H
#cmakedefine HAVE_FUNC_ISINF
#cmakedefine HAVE_FUNC_ROUND
#cmakedefine HAVE_FUNC_ROUNDL
