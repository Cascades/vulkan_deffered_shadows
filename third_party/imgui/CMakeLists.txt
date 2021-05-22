add_library(imgui STATIC
	imgui.cpp
	imgui_demo.cpp
	imgui_draw.cpp
	imgui_impl_glfw.cpp
	imgui_impl_vulkan.cpp
	imgui_tables.cpp
	imgui_widgets.cpp
    )

target_include_directories(imgui PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(imgui
	glfw
	Vulkan::Vulkan
)
