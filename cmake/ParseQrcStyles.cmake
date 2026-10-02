# Parse QSS <file> entries from a Qt .qrc file.
#
# resources/resources.qrc is the single source of truth for which style
# resources ship (AUD-121). UnitTests embeds styles independently via
# qt_add_resources because the test binary does not link the production
# executable; this function derives that list from the qrc instead of
# duplicating it by hand.
#
# parse_qrc_qss_files(<qrc_file> <base_dir> <out_var>)
#   <qrc_file> Path to the .qrc file.
#   <base_dir> Directory that <file> entries are relative to (qrc base).
#   <out_var>  Variable name to receive absolute paths of *.qss entries.
function(parse_qrc_qss_files qrc_file base_dir out_var)
  if(NOT EXISTS "${qrc_file}")
    message(FATAL_ERROR "parse_qrc_qss_files: qrc file not found: ${qrc_file}")
  endif()

  file(READ "${qrc_file}" _qrc_content)
  string(REGEX MATCHALL "<file[^>]*>[^<]*</file>" _file_entries "${_qrc_content}")

  set(_qss_files "")
  foreach(_entry IN LISTS _file_entries)
    string(REGEX REPLACE "<file[^>]*>([^<]*)</file>" "\\1" _rel "${_entry}")
    if(_rel MATCHES "\\.qss$")
      list(APPEND _qss_files "${base_dir}/${_rel}")
    endif()
  endforeach()

  if(NOT _qss_files)
    message(FATAL_ERROR
      "parse_qrc_qss_files: no *.qss <file> entries found in ${qrc_file}")
  endif()

  set(${out_var} "${_qss_files}" PARENT_SCOPE)
endfunction()
