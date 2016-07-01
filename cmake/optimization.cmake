SET(COSMO_DEBUG FALSE CACHE STRING "Turn on debug mode (turn off optimizations, turn on valgrind")
SET(COSMO_PROFILE FALSE CACHE STRING "Turn on profiling (gprof flag)")

# add -pg for gprof
# add -g for valgrind
if(COSMO_DEBUG)
  set(PROFILING     "-g")
  set(OPT_LEVEL     "-O1")
  set(CC_OPTS       "")
else()
  set(PROFILING     "")
  set(OPT_LEVEL     "-O3 -ffast-math")
  set(CC_OPTS       "-march=native")
endif()

if(COSMO_PROFILE)
  set(PROFILING "${PROFILING} -pg")
endif()

set(WARNINGS          "-pedantic -Wall -Wno-unused-variable -Wno-unused-function")
set(CMAKE_CXX_FLAGS   "${CC_OPTS} ${OPT_LEVEL} ${WARNINGS} ${PROFILING}")
set(CMAKE_EXE_LINKER_FLAGS  "${PROFILING}")