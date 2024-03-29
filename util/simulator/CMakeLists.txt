configure_file(hrpsys-simulator-jython.in ${CMAKE_CURRENT_BINARY_DIR}/hrpsys-simulator-jython)
configure_file(hrpsys-simulator-python.in ${CMAKE_CURRENT_BINARY_DIR}/hrpsys-simulator-python)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${LIBXML2_INCLUDE_DIR})
include_directories(${SDL_INCLUDE_DIR})
find_package(PythonLibs)
include_directories(${PYTHON_INCLUDE_DIRS})

set(target hrpsys-simulator)

add_executable(hrpsys-simulator 
  GLscene.cpp 
  BodyState.cpp
  SceneState.cpp
  Simulator.cpp
  main.cpp
  )

target_link_libraries(hrpsys-simulator 
  hrpsysUtil
  )

add_library(hrpsys SHARED 
  GLscene.cpp 
  BodyState.cpp
  SceneState.cpp
  Simulator.cpp
  PySimulator.cpp
  PyBody.cpp
  PyLink.cpp
  PyShape.cpp
  )

target_link_libraries(hrpsys 
  boost_python 
  hrpsysUtil
  ${PYTHON_LIBRARIES}
  )
set_target_properties(hrpsys PROPERTIES PREFIX "")
set_target_properties(hrpsys PROPERTIES SUFFIX ".so")

install(TARGETS ${target}
  RUNTIME DESTINATION bin
  )

install(TARGETS hrpsys
  LIBRARY DESTINATION share/hrpsys/python CONFIGURATIONS Release Debug
)

install(PROGRAMS 
  ${CMAKE_CURRENT_BINARY_DIR}/hrpsys-simulator-jython
  ${CMAKE_CURRENT_BINARY_DIR}/hrpsys-simulator-python
  DESTINATION bin
  )  
