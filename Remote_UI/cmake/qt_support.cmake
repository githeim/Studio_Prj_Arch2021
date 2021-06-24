# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)

SET ( LIBRARY_LISTS ${LIBRARY_LISTS} 
  Qt5::Widgets
    )
 
