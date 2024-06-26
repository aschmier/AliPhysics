#--------------------------------------------------------------------------#
# Set Basic CMake Configuration                                            #
#--------------------------------------------------------------------------#

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(AliPhysics CXX C)
enable_testing()

foreach(p CMP0005  # Properly escape preprocessor definitions (v2.6, v3.0.2)
          CMP0025  # Compiler id for Apple Clang is now AppleClang (v3.0)
          CMP0042  # MACOSX_RPATH is enabled by default (v3.0)
          CMP0053  # Simplify variable reference and escape sequence evaluation (v3.1.3)
          CMP0068  # RPATH settings on macOS do not affect install_name (v3.9 and newer)
       )
  if(POLICY ${p})
    cmake_policy(SET ${p} NEW)
  endif()
endforeach()

message(STATUS "CMake platform: ${CMAKE_SYSTEM}")
message(STATUS "Build folder: ${AliPhysics_BINARY_DIR}")
message(STATUS "Source folder: ${AliPhysics_SOURCE_DIR}")
message(STATUS "Installation folder: ${CMAKE_INSTALL_PREFIX}")

# CMake supports different build types by default. We want the DEBUG build type
# to have "-g -O0" flags: by default it only has "-g"
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

# Build type for coverage builds
set(CMAKE_CXX_FLAGS_COVERAGE "-g -O2 -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_COVERAGE}")
set(CMAKE_LINK_FLAGS_COVERAGE "-fprofile-arcs -fPIC")

MARK_AS_ADVANCED(
    CMAKE_CXX_FLAGS_COVERAGE
    CMAKE_C_FLAGS_COVERAGE
    CMAKE_LINK_FLAGS_COVERAGE)

# You can change the build type using
# cmake -DCMAKE_BUILD_TYPE=DEBUG | RELEASE | RELWITHDEBINFO | MINSIZEREL ...
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELWITHDEBINFO)
endif(NOT CMAKE_BUILD_TYPE)
string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
set(CMAKE_ALLOWED_BUILD_TYPES DEBUG RELEASE RELWITHDEBINFO COVERAGE CUSTOM_EXTERNAL)
list(FIND CMAKE_ALLOWED_BUILD_TYPES "${CMAKE_BUILD_TYPE}" VALID_BUILD_TYPE)
if(${VALID_BUILD_TYPE} EQUAL -1)
  string(REPLACE ";" ", " CMAKE_ALLOWED_BUILD_TYPES_FLAT "${CMAKE_ALLOWED_BUILD_TYPES}")
  message(FATAL_ERROR "Invalid build type ${CMAKE_BUILD_TYPE}. Use one of: ${CMAKE_ALLOWED_BUILD_TYPES_FLAT}. Build type is case-sensitive.")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE} (${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}})")

if(CMAKE_INSTALL_PREFIX STREQUAL "${AliPhysics_SOURCE_DIR}")
    message(FATAL_ERROR "Please choose a different installation point than the source tree!")
endif()

# Path to additonal modules
set(CMAKE_MODULE_PATH "${AliPhysics_SOURCE_DIR}/cmake")

# Retrieve Git version and revision
# ALIPHYSICS_VERSION
# ALIPHYSICS_REVISION
# ALIPHYSICS_SERIAL
include(CheckGitVersion)

include(CheckCXXCompilerFlag)

#       - CLANG_MAJOR.CLANG_MINOR or
#       - GCC_MAJOR.GCC_MINOR.GCC_PATCH
include(CheckCompiler)

# Utility to generate PARfiles
include(cmake/GenParFile.cmake)

# Shared library suffix
if (NOT CMAKE_SYSTEM_NAME STREQUAL Windows)
  set(CMAKE_SHARED_LIBRARY_SUFFIX .so)
endif (NOT CMAKE_SYSTEM_NAME STREQUAL Windows)

# Be sure about where libraries and binaries are put
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

# Build targets with install rpath on Mac to dramatically speed up installation
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
  endif()
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()
unset(isSystemDir)

# Optional: Doxygen for reference doc generation
set(DOXYGEN_EXECUTABLE "")
find_package(Doxygen)
if(DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND)
  message(STATUS "Doxygen ${DOXYGEN_VERSION} and Graphviz found: ${DOXYGEN_EXECUTABLE}, ${DOXYGEN_DOT_EXECUTABLE}")
  set(CAN_GENERATE_DOC TRUE)
elseif(DOXYGEN_ONLY)
  message(FATAL_ERROR "Doxygen and Graphviz not found. Cannot generate documentation")
else()
  message(STATUS "Doxygen and Graphviz not found. Disabling support for documentation generation")
endif()

# Doxygen documentation is always included
add_subdirectory(doxygen)

if(DEFINED DOXYGEN_ONLY AND DOXYGEN_ONLY)
  # In this case we do not prepare targets for the rest, but for Doxygen only.
  # This is useful because no checks are performed, and it means that Doxygen
  # documentation can be generated on a relatively "dumb" server that has as
  # requirements only cmake, doxygen and graphviz.
  message(WARNING "Preparing build for Doxygen documentation only as requested. AliPhysics will not be built!")
else()

  # Standard CMake inclusions and checks if we are actually building AliRoot,
  # i.e. not only the documentation

  # AliRoot Core mandatory
  find_package(AliRoot REQUIRED)

  # ROOT configuration mandatory
  if(ROOTSYS)
      find_package(ROOT REQUIRED)

      # ROOT must be built with XML2 support
      if(NOT ROOT_HASXML)
          message(FATAL_ERROR "ROOT was not built with xml2 support. Please reinstall or rebuild ROOT with xml2 support")
      endif(NOT ROOT_HASXML)

      # Issue a warning if ROOT does not have AliEn support (this is wrong in most cases)
      if(NOT ROOT_HASALIEN)
          message(WARNING "ROOT has been built without AliEn support: some features might be unavailable!")
      endif(NOT ROOT_HASALIEN)

      # find VMC standalone
      # VMC standalone will fail if ROOT was found with VMC
      find_package(VMC)
      if(VMC_FOUND)
        message(STATUS "Using standalone VMC")
        # AliRoot's FindROOT module does not actually provide targets but only does link_libraries(ROOT_LIB_DIR).
        # So AliRoot finds libs by name in that dir but not as a target.
        # On the other hand VMCLibrary is linked against targets ROOT::Core etc which come in with a find_package(ROOT CONFIG).
        # So Let's slightly modify the VMCLibrary target properties...
        set(VMC_DEPS Core EG Geom Physics)
        set_target_properties(VMCLibrary PROPERTIES INTERFACE_LINK_LIBRARIES "${VMC_DEPS}")
        # Alias target before VMC was also found by name "VMC"
        add_library(VMC ALIAS VMCLibrary)
        include_directories(${VMC_INCLUDE_DIRS})
      endif()

  else()
      message(FATAL_ERROR "ROOT installation not found!\nPlease point to the ROOT installation using -DROOTSYS=ROOT_INSTALL_DIR")
  endif(ROOTSYS)

  # Enable C++11 by default if found in ROOT
  if(ROOT_HAS_CXX11 AND NOT DISABLE_CXX11)
    message(STATUS "Enabling C++11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  endif()

  # Enable C++14 by default if found in ROOT
  if(ROOT_HAS_CXX14 AND NOT DISABLE_CXX14)
    message(STATUS "Enabling C++14")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
  endif()

  # Turn some common warnings into errors
  check_cxx_compiler_flag(-Werror=mismatched-new-delete CXX_COMPILER_HAS_MISMATCHED_NEW_DELETE)
  if(CXX_COMPILER_HAS_MISMATCHED_NEW_DELETE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=mismatched-new-delete")
  endif()
  check_cxx_compiler_flag(-Werror=delete-non-virtual-dtor CXX_COMPILER_DELETE_NON_VIRTUAL_DTOR)
  if(CXX_COMPILER_HAS_MISMATCHED_NEW_DELETE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=delete-non-virtual-dtor")
  endif()

  # Turn OFF some -Werrors
  check_cxx_compiler_flag(-Wno-error=strict-aliasing CXX_COMPILER_STRICT_ALIASING)
  if(CXX_COMPILER_STRICT_ALIASING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=strict-aliasing")
  endif()

  # Turn on "all" warnings, but skip missing braces warning
  check_cxx_compiler_flag(-Wall CXX_COMPILER_HAS_WALL)
  if(CXX_COMPILER_HAS_WALL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  endif()
  check_cxx_compiler_flag(-Wno-missing-braces CXX_COMPILER_HAS_WNO_MISSING_BRACES)
  if(CXX_COMPILER_HAS_WNO_MISSING_BRACES)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces")
  endif()

  # Enable -Werror if requested
  if(WARNINGS_AS_ERRORS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    message(WARNING "Treating all C++ compile warnings as errors!")
  endif()

  # Issue a warning if AliRoot Core does not have AliEn support (this is wrong in most cases)
  if(NOT AliRoot_HASALIEN)
      message(WARNING "AliRoot Core has been built without with AliEn support: some features might be unavailable!")
  endif()

  # ROOT dictionaries and maps
  include(CMakeALICE)

  # Checking first for DIM, DATE, AMORE and daqDA
  # in case ROOT Extra static library needs to be enabled

  # DATE
  # date-config needs DIMDIR and ODIR set
  if(DATE_CONFIG)
      if(DIMDIR AND ODIR)
          find_package(DATE)
      else()
          message(FATAL_ERROR "DATE enabled but no DIMDIR and ODIR set. Please set DIMDIR and ODIR")
      endif()
  endif()

  # If no Fortran, i.e on Windows
  include(CheckLanguage)

  check_language(Fortran)
  if(CMAKE_Fortran_COMPILER)
    enable_language(Fortran OPTIONAL)
    message(STATUS "Fortran compiler is ${CMAKE_Fortran_COMPILER}")
  else()
    message(STATUS "No Fortran support.")
  endif()

  # Optional: Doxygen for reference doc generation
  find_package(Doxygen)
  if(DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND)
      message(STATUS "Doxygen ${DOXYGEN_VERSION} and Graphviz found: ${DOXYGEN_EXECUTABLE}, ${DOXYGEN_DOT_EXECUTABLE}")
      set(CAN_GENERATE_DOC TRUE)
  else()
      message(STATUS "Doxygen and Graphviz not found. Disabling support for documentation generation")
  endif(DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND)

  # FastJet
  find_package(FASTJET)

  # RooUnfold
  find_package(RooUnfold)

  # ZEROMQ
  find_package(ZeroMQ)

  # KFParticke
  find_package(KFParticle)

  # Boost
  find_package(Boost)

  # General flags -> Should be moved into a configuration file
  set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
  # Avoid problems with -fPIE (set automatically by the previous line).
  set(CMAKE_CXX_COMPILE_OPTIONS_PIE "")
  set(CMAKE_C_COMPILE_OPTIONS_PIE "")
  set(CMAKE_Fortran_COMPILE_OPTIONS_PIE "")
  set(CMAKE_NO_SYSTEM_FROM_IMPORTED TRUE)

  # List of modules for which PARfiles are enabled -- use the cache as global scope and clean it
  set(ALIPARFILES "" CACHE INTERNAL "ALIPARFILES" FORCE)
  # same for libraries now
  set(ALILIBSTESTED "" CACHE INTERNAL "ALILIBSTESTED" FORCE)
  include(AddLibraryTested)

  # AliRoot modules
  add_subdirectory(CORRFW)
  if(ZeroMQ_FOUND)
    add_subdirectory(EVEANALYSIS)
  endif(ZeroMQ_FOUND)
  add_subdirectory(EVENTMIX)
  add_subdirectory(HLTANALYSIS)
  add_subdirectory(JETAN)
  add_subdirectory(OADB)
  add_subdirectory(TENDER)

  # PWG libraries
  add_subdirectory(PWG)
  add_subdirectory(PWGCF)
  add_subdirectory(PWGGA)
  add_subdirectory(PWGDQ)
  add_subdirectory(PWGHF)
  add_subdirectory(PWGJE)
  add_subdirectory(PWGLF)
  add_subdirectory(PWGPP)
  add_subdirectory(PWGUD)
  add_subdirectory(PWGMM)
  add_subdirectory (ML)

  # Transition to Run3
  if(ROOT_VERSION_MAJOR EQUAL 6)
    add_subdirectory(RUN3)
  endif()

  # List modules with PARfiles
  string(REPLACE ";" " " ALIPARFILES_FLAT "${ALIPARFILES}")
  message(STATUS "PARfile target enabled for the following modules: ${ALIPARFILES_FLAT}")

endif(DEFINED DOXYGEN_ONLY AND DOXYGEN_ONLY)

# Test library loading. Best output with:
#   ctest --output-on-failure
# Select only "load_library" tests (which are quick) like this (in parallel too):
#   ctest --output-on-failure -R load_library -j 8
install(DIRECTORY test DESTINATION ${CMAKE_INSTALL_PREFIX})
foreach(TEST_LIB ${ALILIBSTESTED})
  add_test(load_library_${TEST_LIB}
    env
    PATH=$ENV{PATH}
    LD_LIBRARY_PATH=${KFPARTICLE}/lib:${CMAKE_INSTALL_PREFIX}/lib:${ZEROMQ}/lib:${BOOST_ROOT}/lib:${TREELITE_ROOT}/lib:$ENV{LD_LIBRARY_PATH}
    DYLD_LIBRARY_PATH=${KFPARTICLE}/lib:${CMAKE_INSTALL_PREFIX}/lib:${ZEROMQ}/lib:${BOOST_ROOT}/lib:${TREELITE_ROOT}/lib:$ENV{DYLD_LIBRARY_PATH}
    ROOT_HIST=0
    root -n -l -b -q "${CMAKE_INSTALL_PREFIX}/test/load_library/LoadLib.C(\"lib${TEST_LIB}\")")
endforeach()
