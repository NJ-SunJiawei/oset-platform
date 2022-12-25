# Locate APR-Util include paths and libraries

# This module defines
# APRUTIL_INCLUDES, where to find apr.h, etc.
# APRUTIL_LIBS, the libraries to link against to use APR.
# APRUTIL_FOUND, set to yes if found

find_program(APRUTIL_CONFIG_EXECUTABLE
    apu-1-config
    /usr/local/apr/bin
    /usr/local/bin
    /usr/bin
    C:/Progra~1/apr/bin
    )
	
FIND_PATH(APRUtil_INCLUDE_DIRS 
NAMES
    apu.h
PATHS
    /usr/local/apr/include/apr-1
	/usr/apr/include/apr-1
)

find_library(APRUtil_LIBRARIES
NAMES 
    aprutil-1
PATHS 
	/usr/local/apr/lib
	/usr/apr/lib
)

mark_as_advanced(APRUTIL_CONFIG_EXECUTABLE APRUtil_INCLUDE_DIRS APRUtil_LIBRARIES)

macro(_apu_invoke _varname _regexp)
    execute_process(
        COMMAND ${APRUTIL_CONFIG_EXECUTABLE} ${ARGN}
        OUTPUT_VARIABLE _apr_output
        RESULT_VARIABLE _apr_failed
    )

    if(_apr_failed)
        message(FATAL_ERROR "apu-1-config ${ARGN} failed")
    else(_apr_failed)
        string(REGEX REPLACE "[\r\n]"  "" _apr_output "${_apr_output}")
        string(REGEX REPLACE " +$"     "" _apr_output "${_apr_output}")

        if(NOT ${_regexp} STREQUAL "")
            string(REGEX REPLACE "${_regexp}" " " _apr_output "${_apr_output}")
        endif(NOT ${_regexp} STREQUAL "")

        separate_arguments(_apr_output)

        set(${_varname} "${_apr_output}")
    endif(_apr_failed)
endmacro(_apu_invoke)

_apu_invoke(APRUTIL_INCLUDES "(^| )-I"  --includes)
_apu_invoke(APRUTIL_LIBS     ""         --link-ld)

#if(APRUTIL_LIBS AND APRUTIL_INCLUDES)
#    set(APRUTIL_FOUND "YES")
#    set(APRUTIL_DEFINITIONS "")
#    message (STATUS "apr-util found: YES ${APRUTIL_LIBS} ${APRUTIL_INCLUDES}")
#endif(APRUTIL_LIBS AND APRUTIL_INCLUDES)
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(APR DEFAULT_MSG APRUtil_INCLUDE_DIRS APRUtil_LIBRARIES)

IF(APRUtil_FOUND)
   SET( APRUtil_LIBRARIES ${APRUtil_LIBRARIES} )
ELSE(APRUtil_FOUND)
   SET( APRUtil_LIBRARIES )
ENDIF(APRUtil_FOUND)
message (STATUS "aprutil found: YES ${APRUtil_INCLUDE_DIRS} ${APRUtil_LIBRARIES}")
