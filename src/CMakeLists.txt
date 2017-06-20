set(binary_name "${PROJECT_NAME}")
add_executable("${binary_name}" main.c)
target_link_libraries("${binary_name}" m)
