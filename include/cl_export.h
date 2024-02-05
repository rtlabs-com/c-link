
#ifndef CL_EXPORT_H
#define CL_EXPORT_H

#ifdef CL_STATIC_DEFINE
#  define CL_EXPORT
#  define CL_NO_EXPORT
#else
#  ifndef CL_EXPORT
#    ifdef clink_EXPORTS
        /* We are building this library */
#      define CL_EXPORT 
#    else
        /* We are using this library */
#      define CL_EXPORT 
#    endif
#  endif

#  ifndef CL_NO_EXPORT
#    define CL_NO_EXPORT 
#  endif
#endif

#ifndef CL_DEPRECATED
#  define CL_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef CL_DEPRECATED_EXPORT
#  define CL_DEPRECATED_EXPORT CL_EXPORT CL_DEPRECATED
#endif

#ifndef CL_DEPRECATED_NO_EXPORT
#  define CL_DEPRECATED_NO_EXPORT CL_NO_EXPORT CL_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef CL_NO_DEPRECATED
#    define CL_NO_DEPRECATED
#  endif
#endif

#endif /* CL_EXPORT_H */
