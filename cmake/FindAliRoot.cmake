# Check AliRoot installation

# this files has been taken from the AliPhysics repository
# http://git.cern.ch/pub/AliPhysics
#
# Modifications:
# - info message for package-notfound if the package is optional

set(AliRoot_FOUND FALSE)

set(ALIROOT CACHE STRING "AliRoot installation location")
if(ALIROOT)
  # Check for aliroot executable
  find_program(ALIROOT_EXE NAMES aliroot PATHS ${ALIROOT}/bin NO_DEFAULT_PATH)

  if(NOT ALIROOT_EXE)
    set(AliRoot_FOUND FALSE)
    message(WARNING "AliRoot executable not found in: ${ALIROOT}")
  else()
    mark_as_advanced(ALIROOT_EXE)

    include_directories(
      ${ALIROOT}/include
      ${ALIROOT}/include/pythia
    )

    link_directories(${ALIROOT}/lib)

    include(${ALIROOT}/etc/AliRoot-config.cmake)
    message(STATUS "Found AliRoot version : \"${AliRoot_VERSION}\", git hash : \"${AliRoot_REVISION}\"")
    set(AliRoot_FOUND TRUE)

    if(EXISTS ${ALIROOT}/include/AliAnalysisAlien.h)
      set(AliRoot_HASALIEN TRUE)
    else()
      set(AliRoot_HASALIEN FALSE)
    endif()

  endif()
endif(ALIROOT)

if(NOT AliRoot_FOUND)
  if(AliRoot_FIND_REQUIRED)
    message(FATAL_ERROR "Please point to the AliRoot Core installation using -DALIROOT=<ALIROOT_CORE_INSTALL_DIR>")
  else()
    message(STATUS "No AliRoot Core installation found, some code will be disabled, package can be anabled using flag -DALIROOT=<ALIROOT_CORE_INSTALL_DIR>")
  endif(AliRoot_FIND_REQUIRED)
endif(NOT AliRoot_FOUND)
