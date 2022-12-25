# Files used both in ../src and ../tests
set(JUCI_SHARED_FILES
  autocomplete.cpp
  cmake.cpp
  commands.cpp
  config.cpp
  compile_commands.cpp
  ctags.cpp
  dispatcher.cpp
  documentation.cpp
  filesystem.cpp
  git.cpp
  grep.cpp
  json.cpp
  menu.cpp
  meson.cpp
  project_build.cpp
  snippets.cpp
  source.cpp
  source_base.cpp
  source_clang.cpp
  source_diff.cpp
  source_generic.cpp
  source_language_protocol.cpp
  source_spellcheck.cpp
  terminal.cpp
  tooltips.cpp
  usages_clang.cpp
  utility.cpp
)
if(LIBLLDB_FOUND)
  list(APPEND JUCI_SHARED_FILES debug_lldb.cpp)
endif()
add_library(juci_shared STATIC ${JUCI_SHARED_FILES})
target_link_libraries(juci_shared
  ${ASPELL_LIBRARIES}
  ${GTKMM_LIBRARIES}
  ${GTKSVMM_LIBRARIES}
  ${LIBGIT2_LIBRARIES}
  ${LIBLLDB_LIBRARIES}
  Boost::filesystem
  Boost::serialization
  clangmm
  tiny-process-library
)
target_compile_options(juci_shared PUBLIC -DIDE_INSTALL_PATH="${SSET_INSTALL_PATH}")

set(JUCI_FILES
  dialog.cpp
  directories.cpp
  entrybox.cpp
  info.cpp
  juci.cpp
  notebook.cpp
  project.cpp
  selection_dialog.cpp
  window.cpp
)
if(APPLE)
  list(APPEND JUCI_FILES window_macos.m)
endif()

add_executable(openDEV ${JUCI_FILES})
target_link_libraries(openDEV juci_shared)

if(APPLE)
  target_link_libraries(openDEV "-framework Foundation -framework AppKit")
endif()

install(TARGETS openDEV RUNTIME DESTINATION ${SSET_RUN_INSTALL_PATH} COMPONENT sset)
install(TARGETS openDEV RUNTIME DESTINATION bin COMPONENT sset)
