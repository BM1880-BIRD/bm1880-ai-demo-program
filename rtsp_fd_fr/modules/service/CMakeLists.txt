cmake_minimum_required(VERSION 3.0)

init_module("service" INTERFACE)
add_submodule("service" "face_server")
add_submodule("service" "face_client")
add_submodule("service" "utils")
target_link_libraries("service_face_server" PUBLIC service_utils inference images io utility pipeline_extension)
target_link_libraries("service_face_client" PUBLIC service_utils inference_fdfr images io utility pipeline_extension)