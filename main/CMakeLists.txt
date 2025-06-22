set(src_dirs        ./)

set(include_dirs    ./)

set(requires        "fatfs vfs esp_hw_support driver esp-dl")

# list(APPEND requires esp32_p4_function_ev_board_noglib)

set(embed_files     "human_face.jpg" "models/human_face_detect_msr_s8_v1.espdl" "models/human_face_detect_mnp_s8_v1.espdl")

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires} EMBED_FILES ${embed_files})
