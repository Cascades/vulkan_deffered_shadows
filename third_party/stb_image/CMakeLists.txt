add_library(stb_image INTERFACE)

target_include_directories(stb_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stb::image ALIAS stb_image)