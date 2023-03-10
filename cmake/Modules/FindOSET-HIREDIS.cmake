FIND_PATH(OSET_HIREDIS_INCLUDE_DIRS 
NAMES
    mod_hiredis.h
PATHS
    #/usr/local/install/oset/mods/m_hiredis/include
	${CMAKE_INSTALL_PREFIX}/install/oset/mods/m_hiredis/include
)

find_library(OSET_HIREDIS_LIBRARIES
NAMES 
    oset-hiredis
PATHS 
	#/usr/local/install/oset/lib
	${CMAKE_INSTALL_PREFIX}/install/oset/lib
)

mark_as_advanced(OSET_HIREDIS_INCLUDE_DIRS OSET_HIREDIS_LIBRARIES)


INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OSET-HIREDIS DEFAULT_MSG OSET_HIREDIS_INCLUDE_DIRS OSET_HIREDIS_LIBRARIES)

IF(OSET_FOUND)
   SET( OSET_HIREDIS_LIBRARIES ${OSET_HIREDIS_LIBRARIES})
ELSE(OSET_FOUND)
   SET( OSET_HIREDIS_LIBRARIES )
ENDIF(OSET_FOUND)
message (STATUS "OSET-HIREDIS found: YES ${OSET_HIREDIS_INCLUDE_DIRS} ${OSET_HIREDIS_LIBRARIES}")