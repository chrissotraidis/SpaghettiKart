# The game module resolves these runtime APIs from the application at load time.
set(runtime_exports "${SPAGHETTIPAD_SHELL_DIR}/runtime-exports.txt")
if(NOT EXISTS "${runtime_exports}")
  message(FATAL_ERROR "The iOS module host requires its reviewed runtime export list")
endif()

# Input archives normally hide their definitions. Linker export flags alone
# cannot promote those symbols, so expose them during this opt-in build.
function(spaghettipad_runtime_visibility directory)
  get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach(target IN LISTS targets)
    get_target_property(kind ${target} TYPE)
    if(NOT kind MATCHES "INTERFACE_LIBRARY|UTILITY")
      set_target_properties(${target} PROPERTIES
        C_VISIBILITY_PRESET default CXX_VISIBILITY_PRESET default
        VISIBILITY_INLINES_HIDDEN OFF)
      target_compile_options(${target} PRIVATE -fvisibility=default
        "$<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-fno-visibility-inlines-hidden>")
    endif()
  endforeach()
  get_property(children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
  foreach(child IN LISTS children)
    spaghettipad_runtime_visibility("${child}")
  endforeach()
endfunction()
spaghettipad_runtime_visibility("${CMAKE_CURRENT_SOURCE_DIR}")

# Select the needed archive members instead of all_load: Torch and libultraship
# contain some duplicate helpers that must keep normal archive selection.
file(STRINGS "${runtime_exports}" runtime_symbols)
foreach(symbol IN LISTS runtime_symbols)
  if(NOT symbol MATCHES "^_[A-Za-z0-9_]+$")
    message(FATAL_ERROR "Invalid module-host export: ${symbol}")
  endif()
  target_link_options(${PROJECT_NAME} PRIVATE "SHELL:-Xlinker -u -Xlinker ${symbol}")
endforeach()
target_link_options(${PROJECT_NAME} PRIVATE
  "-Wl,-export_dynamic" "-Wl,-exported_symbols_list,${runtime_exports}")
