include_directories(${OCTOMAP_INCLUDE_DIR})
link_directories(${OCTOMAP_DIR}/lib)
set(comp_sources OccupancyGridMap3D.cpp OGMap3DService_impl.cpp)
set(libs ${OPENHRP_LIBRARIES} ${OCTOMAP_LIBRARIES} hrpsysBaseStub)
add_library(OccupancyGridMap3D SHARED ${comp_sources})
target_link_libraries(OccupancyGridMap3D ${libs})
set_target_properties(OccupancyGridMap3D PROPERTIES PREFIX "")

add_executable(OccupancyGridMap3DComp OccupancyGridMap3DComp.cpp ${comp_sources})
target_link_libraries(OccupancyGridMap3DComp ${libs})

set(target OccupancyGridMap3D OccupancyGridMap3DComp)

install(TARGETS ${target}
  RUNTIME DESTINATION bin CONFIGURATIONS Release Debug
  LIBRARY DESTINATION lib CONFIGURATIONS Release Debug
)
