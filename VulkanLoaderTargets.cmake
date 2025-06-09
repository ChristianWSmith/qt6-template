# Load the debug and release variables
file(GLOB DATA_FILES "${CMAKE_CURRENT_LIST_DIR}/VulkanLoader-*-data.cmake")

foreach(f ${DATA_FILES})
    include(${f})
endforeach()

# Create the targets for all the components
foreach(_COMPONENT ${vulkan-loader_COMPONENT_NAMES} )
    if(NOT TARGET ${_COMPONENT})
        add_library(${_COMPONENT} INTERFACE IMPORTED)
        message(${VulkanLoader_MESSAGE_MODE} "Conan: Component target declared '${_COMPONENT}'")
    endif()
endforeach()

if(NOT TARGET Vulkan::Loader)
    add_library(Vulkan::Loader INTERFACE IMPORTED)
    message(${VulkanLoader_MESSAGE_MODE} "Conan: Target declared 'Vulkan::Loader'")
endif()
# Load the debug and release library finders
file(GLOB CONFIG_FILES "${CMAKE_CURRENT_LIST_DIR}/VulkanLoader-Target-*.cmake")

foreach(f ${CONFIG_FILES})
    include(${f})
endforeach()