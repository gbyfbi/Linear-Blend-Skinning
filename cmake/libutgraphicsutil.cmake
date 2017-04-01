AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/lib libutgu_src)
ADD_LIBRARY(utgraphicsutil STATIC ${libutgu_src})
list(APPEND stdgl_libraries utgraphicsutil)
