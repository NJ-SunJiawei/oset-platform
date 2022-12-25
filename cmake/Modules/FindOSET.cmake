FIND_PATH(OSET_INCLUDE_DIRS 
NAMES
    oset-core.h
PATHS
    #/usr/local/install/oset/base/include
	${CMAKE_INSTALL_PREFIX}/install/oset/base/include
)

find_library(OSET_LIBRARIES
NAMES 
    oset
PATHS 
	#/usr/local/install/oset/lib
	${CMAKE_INSTALL_PREFIX}/install/oset/lib
)

mark_as_advanced(OSET_INCLUDE_DIRS OSET_LIBRARIES)


INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OSET DEFAULT_MSG OSET_INCLUDE_DIRS OSET_LIBRARIES)

IF(OSET_FOUND)
   SET( OSET_LIBRARIES ${OSET_LIBRARIES} )
ELSE(OSET_FOUND)
   SET( OSET_LIBRARIES )
ENDIF(OSET_FOUND)
message (STATUS "OSET found: YES ${OSET_INCLUDE_DIRS} ${OSET_LIBRARIES}")