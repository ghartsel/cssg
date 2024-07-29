
configure_file(cssg_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/cssg_version.h)
configure_file(libcssg.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/libcssg.pc
  @ONLY)

add_library(cssg
  blocks.c
  buffer.c
  cssg.c
  cssg_ctype.c
  commonmark.c
  houdini_href_e.c
  houdini_html_e.c
  houdini_html_u.c
  html.c
  inlines.c
  iterator.c
  latex.c
  man.c
  node.c
  references.c
  render.c
  scanners.c
  scanners.re
  utf8.c
  xml.c)
cssg_add_compile_options(cssg)
set_target_properties(cssg PROPERTIES
  OUTPUT_NAME "cssg"
  # Avoid name clash between PROGRAM and LIBRARY pdb files.
  PDB_NAME libcssg
  POSITION_INDEPENDENT_CODE YES
  # Include minor version and patch level in soname for now.
  SOVERSION ${PROJECT_VERSION}
  VERSION ${PROJECT_VERSION})
if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(cssg PUBLIC
    CSSG_STATIC_DEFINE)
endif()
target_include_directories(cssg INTERFACE
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

generate_export_header(cssg
  BASE_NAME ${PROJECT_NAME})

# FIXME(compnerd) this should be removed, but exists solely to allow a migration
# path for OSS Fuzz.
add_custom_target(cssg_static DEPENDS cssg)

add_executable(cssg_exe
  main.c)
cssg_add_compile_options(cssg_exe)
set_target_properties(cssg_exe PROPERTIES
  OUTPUT_NAME "cssg"
  INSTALL_RPATH "${Base_rpath}")
target_link_libraries(cssg_exe PRIVATE
  cssg)

install(TARGETS cssg_exe cssg
  EXPORT cssg-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libcssg.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(FILES
  cssg.h
  ${CMAKE_CURRENT_BINARY_DIR}/cssg_export.h
  ${CMAKE_CURRENT_BINARY_DIR}/cssg_version.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

# generate cssg-config.cmake and cssg-config-version.cmake files
configure_package_config_file(
  "cssgConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/cssg-config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cssg")
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/generated/cssg-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)
# install config and version file
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/cssg-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/cssg-config-version.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cssg"
)
# install targets file
install(
  EXPORT "cssg-targets"
  NAMESPACE "cssg::"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cssg"
)
