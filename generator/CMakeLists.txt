set(LIBRARY_NAME Generator)

include_directories(
)

link_directories(
)

set(SOURCES
  CollisionDistribution.cxx
  GeneratorTF.cxx
)

if(AliRoot_FOUND)
set(SOURCES
  ${SOURCES}
  ChannelMerger.cxx
)
endif(AliRoot_FOUND)

string (REGEX REPLACE "\\.cxx" ".obj" OBJECTS "${SOURCES}")

set(DEPENDENCIES
  ${DEPENDENCIES}
)

#set(DEPENDENCIES
#  ${DEPENDENCIES}
#   boost_thread boost_system boost_chrono
#)

add_library(
  ${LIBRARY_NAME}
  SHARED
  ${SOURCES}
  )

target_link_libraries( ${LIBRARY_NAME}
  ${DEPENDENCIES}
)

install(TARGETS ${LIBRARY_NAME} DESTINATION lib)

Set(Exe_Names
)

set(Exe_Source
)

list(LENGTH Exe_Names _length)
if(${_length} GREATER 0)
math(EXPR _length ${_length}-1)

ForEach(_file RANGE 0 ${_length})
  list(GET Exe_Names ${_file} _name)
  list(GET Exe_Source ${_file} _src)
  set(EXE_NAME ${_name})
  set(SRCS ${_src})
  set(DEPENDENCIES ${LIBRARY_NAME} dl)
  add_executable(${EXE_NAME} ${SRCS})
  target_link_libraries(${EXE_NAME} ${DEPENDENCIES})
  install(TARGETS ${EXE_NAME} DESTINATION bin)
EndForEach(_file RANGE 0 ${_length})
endif(${_length} GREATER 0)
