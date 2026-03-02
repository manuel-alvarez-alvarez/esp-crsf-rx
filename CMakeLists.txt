idf_component_register(
        SRCS "src/crsf_rx.c" "src/crsf_parser.c"
        INCLUDE_DIRS "include"
        PRIV_INCLUDE_DIRS "src"
        REQUIRES driver
)