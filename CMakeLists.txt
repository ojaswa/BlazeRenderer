cmake_minimum_required(VERSION 3.5.0)

project(BlazeRenderer)
set(TARGET ${CMAKE_PROJECT_NAME})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Dependencies
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})
find_package(OpenGL REQUIRED)

# Qt5 setup
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5PrintSupport REQUIRED)

#set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
set(SOURCES 
	"src/main.cpp" 
	"src/ui/mainwindow.cpp" 
	"src/ui/glwidget.cpp" 
	"src/ui/trackball.cpp" 
	"src/algorithm/volumemanager.cpp" 
	"src/ui/dialog1dtransferfunction.cpp" 
	"depends/qcustomplot/qcustomplot.cpp" 
	"src/ui/dialograycastingsettings.cpp"
	)
set(HEADERS 
	"src/ui/mainwindow.h" 
	"src/ui/glwidget.h" 
	"src/ui/trackball.h" 
	"src/algorithm/volumemanager.h" 
	"src/ui/dialog1dtransferfunction.h" 
	"depends/qcustomplot/qcustomplot.h" 
	"src/ui/dialograycastingsettings.h" 
	"src/defines.h"
	)
set(UI_SOURCES
	"src/ui/mainwindow.ui"
	"src/ui/dialog1dtransferfunction.ui"
	"src/ui/dialograycastingsettings.ui"
	)
set(RESOURCES
	"res/icons.qrc"
	"res/shaders.qrc"
	)

add_executable(${TARGET} ${SOURCES} ${HEADERS} ${UI_SOURCES} ${RESOURCES})
target_include_directories(${TARGET} PRIVATE
	${PROJECT_SOURCE_DIR} 
	${PROJECT_SOURCE_DIR}/src 
	${PROJECT_SOURCE_DIR}/depends/qcustomplot 
	${PROJECT_SOURCE_DIR}/depends/OpenGL
	)
qt5_use_modules(${TARGET} Widgets Concurrent OpenGL PrintSupport)
target_link_libraries(${TARGET} ${ITK_LIBRARIES} ${VTK_LIBRARIES} ${OPENGL_LIBRARIES})
