cmake_minimum_required(VERSION 3.16)

project(Boox VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatically handle Qt's MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 or Qt5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/floatingzone.cpp
    src/floatingzone.h
    src/features/dragdrop/dragdrop.cpp
    src/features/dragdrop/dragdrop.h
    src/features/fileops/fileops.cpp
    src/features/fileops/fileops.h
    src/features/contextmenu/contextmenu.cpp
    src/features/contextmenu/contextmenu.h
)

# Create executable
# WIN32 flag creates a Windows GUI application (Qt handles entry point automatically)
if(WIN32)
    add_executable(Boox WIN32 ${SOURCES})
else()
    add_executable(Boox ${SOURCES})
endif()

# Link Qt libraries
target_link_libraries(Boox
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

# Set output directory to project folder
set_target_properties(Boox PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/build/bin/debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/build/bin/release"
)

# Windows-specific settings
if(WIN32)
    # Add Windows libraries
    target_link_libraries(Boox user32)
    
    # Find windeployqt for MinGW
    if(MINGW)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS 
            "D:/Qt/Qt5.14.2/5.14.2/mingw73_64/bin"
            "C:/Qt/5.14.2/mingw73_64/bin"
        )
    else()
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS
            "D:/Qt/Qt5.14.2/5.14.2/msvc2017_64/bin"
            "C:/Qt/5.14.2/msvc2017_64/bin"
        )
    endif()
    
    # Auto deploy Qt libraries for Release builds
    if(WINDEPLOYQT_EXECUTABLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
        # Get Qt binary directory
        get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
        get_filename_component(QT_BIN_DIR ${QT_QMAKE_EXECUTABLE} DIRECTORY)
        
        # Copy Qt DLLs and plugins manually then run windeployqt
        add_custom_command(TARGET Boox POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_BIN_DIR}/Qt5Core.dll $<TARGET_FILE_DIR:Boox>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_BIN_DIR}/Qt5Gui.dll $<TARGET_FILE_DIR:Boox>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_BIN_DIR}/Qt5Widgets.dll $<TARGET_FILE_DIR:Boox>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_BIN_DIR}/libEGL.dll $<TARGET_FILE_DIR:Boox>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_BIN_DIR}/libGLESv2.dll $<TARGET_FILE_DIR:Boox>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_BIN_DIR}/Qt5Svg.dll $<TARGET_FILE_DIR:Boox>
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Boox>/platforms
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_BIN_DIR}/../plugins/platforms/qwindows.dll $<TARGET_FILE_DIR:Boox>/platforms/
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Boox>/imageformats
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_BIN_DIR}/../plugins/imageformats/qsvg.dll $<TARGET_FILE_DIR:Boox>/imageformats/
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Boox>/iconengines
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_BIN_DIR}/../plugins/iconengines/qsvgicon.dll $<TARGET_FILE_DIR:Boox>/iconengines/
            COMMENT "Deploying Qt libraries manually"
        )
        
        # Also copy MinGW runtime libraries if using MinGW
        if(MINGW)
            # Find MinGW runtime libraries
            find_library(LIBGCC libgcc_s_seh-1 HINTS
                "D:/Qt/Qt5.14.2/Tools/mingw730_64/bin"
                "D:/mingw64/bin"
                "C:/mingw64/bin"
                "C:/msys64/mingw64/bin"
            )
            find_library(LIBSTDCPP libstdc++-6 HINTS
                "D:/Qt/Qt5.14.2/Tools/mingw730_64/bin"
                "D:/mingw64/bin"
                "C:/mingw64/bin"
                "C:/msys64/mingw64/bin"
            )
            find_library(LIBWINPTHREAD libwinpthread-1 HINTS
                "D:/Qt/Qt5.14.2/Tools/mingw730_64/bin"
                "D:/mingw64/bin"
                "C:/mingw64/bin"
                "C:/msys64/mingw64/bin"
            )
            
            if(LIBGCC AND LIBSTDCPP AND LIBWINPTHREAD)
                add_custom_command(TARGET Boox POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBGCC} $<TARGET_FILE_DIR:Boox>
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBSTDCPP} $<TARGET_FILE_DIR:Boox>
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBWINPTHREAD} $<TARGET_FILE_DIR:Boox>
                    COMMENT "Copying MinGW runtime libraries"
                )
            endif()
        endif()
    endif()
endif()

# Installation rules
install(TARGETS Boox
    RUNTIME DESTINATION bin
)


# Print configuration information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt version: ${QT_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
