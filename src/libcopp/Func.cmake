function(EchoWithColor)
  # ${ARGV}, ${ARGN}

  set(ECHO_WITH_COLOR_COLOR "")
  set(ECHO_WITH_COLOR_FLAG NO)
  set(ECHO_WITH_COLOR_MSG "")

  foreach(msg IN LISTS ARGV)
    if(${msg} STREQUAL "COLOR")
      set(ECHO_WITH_COLOR_FLAG YES)
    elseif(ECHO_WITH_COLOR_FLAG)
      set(ECHO_WITH_COLOR_FLAG NO)
      set(ECHO_WITH_COLOR_COLOR ${msg})
    else()
      set(ECHO_WITH_COLOR_MSG "${ECHO_WITH_COLOR_MSG}${msg}")
    endif()
  endforeach()

  if(ECHO_WITH_COLOR_COLOR
     AND Python_EXECUTABLE
     AND ECHO_WITH_COLOR_TOOL_PATH)
    execute_process(COMMAND ${Python_EXECUTABLE} ${ECHO_WITH_COLOR_TOOL_PATH} -e -c ${ECHO_WITH_COLOR_COLOR} "{0}\r\n"
                            "${ECHO_WITH_COLOR_MSG}")
  else()
    message(${ECHO_WITH_COLOR_MSG})
  endif()
endfunction(EchoWithColor)