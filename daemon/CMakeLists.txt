project(daemon)

set(CMAKE_CXX_STANDARD 17)

add_executable(graceful-audit-daemon main.c
        common-x.h common-x.c
        forbid-drag.h forbid-drag.c
        forbid-copy-paste.h forbid-copy-paste.c
)
target_link_libraries(graceful-audit-daemon -lX11 -lXmu ${GLIB_LIBRARIES} )
target_include_directories(graceful-audit-daemon PUBLIC ${GLIB_INCLUDE_DIRS} )
