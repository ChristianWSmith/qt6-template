########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(Vulkan_FIND_QUIETLY)
    set(Vulkan_MESSAGE_MODE VERBOSE)
else()
    set(Vulkan_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/module-VulkanTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${vulkan-loader_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(Vulkan_VERSION_STRING "1.3.268.0")
set(Vulkan_INCLUDE_DIRS ${vulkan-loader_INCLUDE_DIRS_RELEASE} )
set(Vulkan_INCLUDE_DIR ${vulkan-loader_INCLUDE_DIRS_RELEASE} )
set(Vulkan_LIBRARIES ${vulkan-loader_LIBRARIES_RELEASE} )
set(Vulkan_DEFINITIONS ${vulkan-loader_DEFINITIONS_RELEASE} )


# Only the last installed configuration BUILD_MODULES are included to avoid the collision
foreach(_BUILD_MODULE ${vulkan-loader_BUILD_MODULES_PATHS_RELEASE} )
    message(${Vulkan_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


include(FindPackageHandleStandardArgs)
set(Vulkan_FOUND 1)
set(Vulkan_VERSION "1.3.268.0")

find_package_handle_standard_args(Vulkan
                                  REQUIRED_VARS Vulkan_VERSION
                                  VERSION_VAR Vulkan_VERSION)
mark_as_advanced(Vulkan_FOUND Vulkan_VERSION)

set(Vulkan_FOUND 1)
set(Vulkan_VERSION "1.3.268.0")
mark_as_advanced(Vulkan_FOUND Vulkan_VERSION)

