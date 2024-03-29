set(idl_files 
  DataLoggerService.idl 
  StateHolderService.idl 
  RobotHardwareService.idl 
  WavPlayerService.idl 
  SequencePlayerService.idl 
  ForwardKinematicsService.idl
  ExecutionProfileService.idl
  OGMap3DService.idl
  NullService.idl
  TimeKeeperService.idl
  Img.idl
  HRPDataTypes.idl
  pointcloud.idl
  ImpedanceControllerService.idl
  SoftErrorLimiterService.idl
  CollisionDetectorService.idl
  GraspControllerService.idl
  )

set(openhrp_idl_files
  ClockGenerator.idl
  CollisionDetector.idl
  Controller.idl
  DynamicsSimulator.idl
  ModelLoader.idl
  OnlineViewer.idl
  OpenHRPCommon.idl
  PathPlanner.idl
  ViewSimulator.idl  
  World.idl
  )

set(idl_flags -bcxx -Wbuse_quotes -Wbh=.hh -Wbs=Sk.cpp -Wba -Wbd=DynSk.cpp -I${OPENRTM_IDL_DIR})

macro(generate_cpp_stub idl_basename)
  set(idl_file ${CMAKE_CURRENT_SOURCE_DIR}/${idl_basename}.idl)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${idl_basename}.hh ${CMAKE_CURRENT_BINARY_DIR}/${idl_basename}Sk.cpp ${CMAKE_CURRENT_BINARY_DIR}/${idl_basename}DynSk.cpp
    COMMAND omniidl ${idl_flags} ${idl_file}
    DEPENDS ${idl_file}
    )
endmacro()

macro(generate_java_stub idl_basename)
  set(idl_file ${CMAKE_CURRENT_SOURCE_DIR}/${idl_basename}.idl)
  add_custom_command(
    OUTPUT ${idl_basename}.compiled src
    COMMAND ${IDLJ} ${idlj_flags} -I${OPENRTM_IDL_DIR} ${idl_file}
    COMMAND ${CMAKE_COMMAND} -E echo "" > ${idl_basename}.compiled
    DEPENDS ${idl_file}
    )
endmacro()

foreach(idl_file ${idl_files})
  get_filename_component(idl_basename ${idl_file} NAME_WE)
  generate_cpp_stub(${idl_basename})
  list(APPEND sources ${idl_basename}Sk.cpp ${idl_basename}DynSk.cpp)
  list(APPEND headers ${CMAKE_CURRENT_BINARY_DIR}/${idl_basename}.hh)
  generate_java_stub(${idl_basename})
  list(APPEND compiled ${idl_basename}.compiled)
endforeach()

set(jarfile ${CMAKE_CURRENT_BINARY_DIR}/hrpsysBaseStub.jar)

add_custom_command(
  OUTPUT ${jarfile} cls
  COMMAND mkdir -p cls
  COMMAND ${JAVAC} ${javac_flags} src/*/*.java -d cls
  COMMAND ${JAR} cf ${jarfile} -C ${CMAKE_CURRENT_BINARY_DIR}/cls Img -C ${CMAKE_CURRENT_BINARY_DIR}/cls OpenHRP -C ${CMAKE_CURRENT_BINARY_DIR}/cls OpenRTM -C ${CMAKE_CURRENT_BINARY_DIR}/cls RTC -C ${CMAKE_CURRENT_BINARY_DIR}/cls _SDOPackage
  COMMAND cp ${jarfile} ${CMAKE_SOURCE_DIR}/plugin
  DEPENDS ${compiled}
  )
add_custom_target(StubJava ALL DEPENDS ${jarfile})

set(target hrpsysBaseStub)
add_library(${target} SHARED ${sources})
target_link_libraries(${target} ${OPENRTM_LIBRARIES})

install(TARGETS ${target}
	LIBRARY DESTINATION lib CONFIGURATIONS Release Debug
)

install(FILES ${jarfile} DESTINATION share/hrpsys/jar)
install(CODE "execute_process(COMMAND omniidl -bpython -C${CMAKE_INSTALL_PREFIX}/share/hrpsys/python -I${OPENRTM_IDL_DIR} ${idl_files} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")
install(CODE "execute_process(COMMAND omniidl -bpython -C${CMAKE_INSTALL_PREFIX}/share/hrpsys/python -I${OPENRTM_IDL_DIR} ${openhrp_idl_files} WORKING_DIRECTORY ${OPENHRP_DIR}/share/OpenHRP-3.1/idl/OpenHRP)")
install(FILES ${headers} DESTINATION include/hrpsys/idl)
install(FILES ${idl_files} DESTINATION share/hrpsys/idl)

