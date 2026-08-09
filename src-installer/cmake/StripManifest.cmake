if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
  message(FATAL_ERROR "INPUT and OUTPUT are required")
endif()

file(READ "${INPUT}" resource_text)
string(FIND "${resource_text}" "/* Type: version" metadata_offset)
if(metadata_offset EQUAL -1)
  message(FATAL_ERROR "VERSIONINFO was not found in converted resource")
endif()
string(SUBSTRING "${resource_text}" 0 ${metadata_offset} resource_payloads)

# Replace selected legacy RCDATA payloads with maintained/build outputs.  Each
# block is located by the stable comments emitted by GNU windres.
function(replace_rcdata resource_name filename)
  if(NOT EXISTS "${filename}")
    message(FATAL_ERROR "${resource_name} replacement does not exist: ${filename}")
  endif()
  set(text "${resource_payloads}")
  set(marker "/* Name: \"${resource_name}\".  */")
  string(FIND "${text}" "${marker}" block_start)
  if(block_start EQUAL -1)
    message(FATAL_ERROR "${resource_name} resource was not found")
  endif()
  string(SUBSTRING "${text}" ${block_start} -1 from_block)
  string(LENGTH "${marker}" marker_length)
  string(SUBSTRING "${from_block}" ${marker_length} -1 after_marker)
  string(FIND "${after_marker}" "/* Name: \"" next_relative)
  if(next_relative EQUAL -1)
    string(LENGTH "${text}" block_end)
  else()
    math(EXPR block_end "${block_start} + ${marker_length} + ${next_relative}")
  endif()
  string(SUBSTRING "${text}" 0 ${block_start} before_block)
  string(SUBSTRING "${text}" ${block_end} -1 after_block)
  file(TO_CMAKE_PATH "${filename}" payload_path)
  string(REPLACE "\"" "\\\"" payload_path "${payload_path}")
  set(resource_payloads
      "${before_block}LANGUAGE 0, 0\n${resource_name} RCDATA \"${payload_path}\"\n\n${after_block}"
      PARENT_SCOPE)
endfunction()

if(DEFINED CONFIG_INI AND NOT CONFIG_INI STREQUAL "")
  if(NOT EXISTS "${CONFIG_INI}")
    message(FATAL_ERROR "CONFIG replacement does not exist: ${CONFIG_INI}")
  endif()
  string(FIND "${resource_payloads}" "/* Type: rcdata" config_start)
  string(FIND "${resource_payloads}" "/* Name: \"LICENSE\".  */" config_end)
  if(config_start EQUAL -1 OR config_end EQUAL -1 OR config_end LESS config_start)
    message(FATAL_ERROR "CONFIG resource boundaries were not found")
  endif()
  string(SUBSTRING "${resource_payloads}" 0 ${config_start} before_config)
  string(SUBSTRING "${resource_payloads}" ${config_end} -1 after_config)
  file(TO_CMAKE_PATH "${CONFIG_INI}" config_path)
  string(REPLACE "\"" "\\\"" config_path "${config_path}")
  set(resource_payloads
      "${before_config}LANGUAGE 0, 0\nCONFIG RCDATA \"${config_path}\"\n\n${after_config}")
endif()
if(DEFINED RDPW32 AND NOT RDPW32 STREQUAL "")
  replace_rcdata("RDPW32" "${RDPW32}")
endif()
if(DEFINED RDPW64 AND NOT RDPW64 STREQUAL "")
  replace_rcdata("RDPW64" "${RDPW64}")
endif()
if(DEFINED RDP_CNC AND NOT RDP_CNC STREQUAL "")
  replace_rcdata("RDP_CNC" "${RDP_CNC}")
endif()
# GNU windres emits quoted string resource identifiers. Microsoft rc.exe keeps
# those quote characters as part of the identifier, so FindResource("LICENSE")
# cannot find a resource named "\"LICENSE\"". All existing RCDATA names are
# valid RC identifiers; normalize them for identical MSVC/MinGW resource trees.
string(REGEX REPLACE
  "\"([A-Za-z_][A-Za-z0-9_]*)\"([ \t]+RCDATA)"
  "\\1\\2" resource_payloads "${resource_payloads}")
file(WRITE "${OUTPUT}" "${resource_payloads}
LANGUAGE 9, 1
1 VERSIONINFO
 FILEVERSION 1,8,7,0
 PRODUCTVERSION 1,8,7,0
 FILEFLAGSMASK 0x3fL
 FILEOS 0x4L
 FILETYPE 0x1L
BEGIN
  BLOCK \"StringFileInfo\"
  BEGIN
    BLOCK \"040904B0\"
    BEGIN
      VALUE \"FileDescription\", \"RDP Wrapper Library Installer\"
      VALUE \"FileVersion\", \"1.8.7.0\"
      VALUE \"InternalName\", \"RDPWInst\"
      VALUE \"OriginalFilename\", \"RDPWInst.exe\"
      VALUE \"ProductName\", \"RDP Host Support\"
      VALUE \"ProductVersion\", \"1.8.7.0\"
    END
  END
  BLOCK \"VarFileInfo\"
  BEGIN
    VALUE \"Translation\", 0x409, 1200
  END
END
")
