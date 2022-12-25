cmake_minimum_required(VERSION 3.5)

set(geh_SOURCES
    dir.c
    file_fetch.c
    file_fetch_img.c
    file_multi.c
    file_queue.c
    image.c
    md5.c
    orientation.c
    thumb.c
    ui_window.c
    util.c
    main.c)

add_executable(geh ${geh_SOURCES})
add_definitions(-DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE)
target_include_directories(geh PUBLIC ${GTK_INCLUDE_DIRS})
target_link_libraries(geh ${GTK_LINK_LIBRARIES})

install(TARGETS geh DESTINATION bin)
