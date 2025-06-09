message(STATUS "Conan: Using CMakeDeps conandeps_legacy.cmake aggregator via include()")
message(STATUS "Conan: It is recommended to use explicit find_package() per dependency instead")

find_package(fmt)
find_package(VulkanLoader)
find_package(VulkanHeaders)

set(CONANDEPS_LEGACY  fmt::fmt  Vulkan::Loader  vulkan-headers::vulkan-headers )