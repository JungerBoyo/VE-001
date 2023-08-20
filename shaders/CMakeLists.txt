if(REMOTE_BUILD)
  add_custom_target(compile_and_copy_shaders_dirs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/bin ${CMAKE_CURRENT_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src
  )
else()
  set(COMPILE_SHADERS ninja -C ${CMAKE_CURRENT_SOURCE_DIR})
  add_custom_target(compile_and_copy_shaders_dirs
    COMMAND ${COMPILE_SHADERS}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/bin ${CMAKE_CURRENT_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src
  )
endif()