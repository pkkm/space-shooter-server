set(binary_name "${PROJECT_NAME}")
add_executable("${binary_name}"
  main.c color.c  cpsock.c  cptime.c  rnd.c  serialization.c  vec2f.c  vector.c)
target_link_libraries("${binary_name}" m)
