#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "jetson-utils" for configuration ""
set_property(TARGET jetson-utils APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(jetson-utils PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_NOCONFIG "/usr/local/cuda/lib64/libcudart_static.a;-lpthread;dl;/usr/lib/aarch64-linux-gnu/librt.so;GL;GLU;GLEW;gstreamer-1.0;gstapp-1.0;gstpbutils-1.0;/usr/local/cuda/lib64/libnppicc.so"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libjetson-utils.so"
  IMPORTED_SONAME_NOCONFIG "libjetson-utils.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS jetson-utils )
list(APPEND _IMPORT_CHECK_FILES_FOR_jetson-utils "${_IMPORT_PREFIX}/lib/libjetson-utils.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
