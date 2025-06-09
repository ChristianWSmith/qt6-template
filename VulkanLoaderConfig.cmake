########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(VulkanLoader_FIND_QUIETLY)
    set(VulkanLoader_MESSAGE_MODE VERBOSE)
else()
    set(VulkanLoader_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/VulkanLoaderTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${vulkan-loader_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(VulkanLoader_VERSION_STRING "1.3.268.0")
set(VulkanLoader_INCLUDE_DIRS ${vulkan-loader_INCLUDE_DIRS_RELEASE} )
set(VulkanLoader_INCLUDE_DIR ${vulkan-loader_INCLUDE_DIRS_RELEASE} )
set(VulkanLoader_LIBRARIES ${vulkan-loader_LIBRARIES_RELEASE} )
set(VulkanLoader_DEFINITIONS ${vulkan-loader_DEFINITIONS_RELEASE} )


# Only the last installed configuration BUILD_MODULES are included to avoid the collision
foreach(_BUILD_MODULE ${vulkan-loader_BUILD_MODULES_PATHS_RELEASE} )
    message(${VulkanLoader_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


