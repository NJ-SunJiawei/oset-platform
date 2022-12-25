# Locate APR include paths and libraries

# This module defines
# APR_INCLUDES, where to find apr.h, etc.
# APR_LIBS, the libraries to link against to use APR.
# APR_FLAGS, the flags to use to compile
# APR_DEFINITIONS, definitions to use when compiling code that uses APR.
# APR_FOUND, set to 'yes' if found

find_program(APR_CONFIG_EXECUTABLE
    apr-1-config
    /usr/local/apr/bin
    /usr/local/bin
    /usr/bin
    C:/Progra~1/apr/bin
    )

FIND_PATH(APR_INCLUDE_DIRS 
NAMES
    apr.h
PATHS
    /usr/local/apr/include/apr-1
	/usr/apr/include/apr-1
)

find_library(APR_LIBRARIES
NAMES 
    apr-1
PATHS 
	/usr/local/apr/lib
	/usr/apr/lib
)

mark_as_advanced(APR_CONFIG_EXECUTABLE APR_INCLUDE_DIRS APR_LIBRARIES)

macro(_apr_invoke _varname _regexp)
    execute_process(
        COMMAND ${APR_CONFIG_EXECUTABLE} ${ARGN}
        OUTPUT_VARIABLE _apr_output
        RESULT_VARIABLE _apr_failed
    )

    if(_apr_failed)
        message(FATAL_ERROR "apr-1-config ${ARGN} failed")
    else(_apr_failed)
        string(REGEX REPLACE "[\r\n]"  "" _apr_output "${_apr_output}")
        string(REGEX REPLACE " +$"     "" _apr_output "${_apr_output}")

        if(NOT ${_regexp} STREQUAL "")
            string(REGEX REPLACE "${_regexp}" " " _apr_output "${_apr_output}")
        endif(NOT ${_regexp} STREQUAL "")

        separate_arguments(_apr_output)
        set(${_varname} "${_apr_output}")
    endif(_apr_failed)
endmacro(_apr_invoke)

_apr_invoke(APR_INCLUDES  "(^| )-I" --includes)
_apr_invoke(APR_FLAGS               --cppflags --cflags)
_apr_invoke(APR_EXTRALIBS "(^| )-l" --libs)
_apr_invoke(APR_LIBS      ""        --link-ld)
#if(APR_INCLUDES AND APR_EXTRALIBS AND APR_LIBS)
#    set(APR_FOUND "YES")
#    message (STATUS "apr found: YES ${APR_LIBS} ${APR_INCLUDES}")
#endif(APR_INCLUDES AND APR_EXTRALIBS AND APR_LIBS)
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(APR DEFAULT_MSG APR_INCLUDE_DIRS APR_LIBRARIES)

IF(APR_FOUND)
   SET( APR_LIBRARIES ${APR_LIBRARIES} )
ELSE(APR_FOUND)
   SET( APR_LIBRARIES )
ENDIF(APR_FOUND)
message (STATUS "apr found: YES ${APR_INCLUDE_DIRS} ${APR_LIBRARIES}")