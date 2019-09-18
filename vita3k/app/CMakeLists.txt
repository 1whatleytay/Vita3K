if (USE_VULKAN)
	set(APP_VULKAN_SOURCES
		include/app/vulkan_screen_renderer.h

		src/vulkan_screen_renderer.cpp)
else()
	set(APP_VULKAN_SOURCES "")
endif()

add_library(
	app
	STATIC
	include/app/functions.h
	include/app/discord.h
	include/app/screen_rendererer.h
	include/app/gl_screen_rendererer.h
	src/app_init.cpp
	src/app.cpp
	src/discord.cpp
	src/gl_screen_rendererer.cpp
	${APP_VULKAN_SOURCES}
)

target_include_directories(app PUBLIC include)
target_link_libraries(app PUBLIC discord-rpc host vita-headers renderer)
target_link_libraries(app PRIVATE gui)
