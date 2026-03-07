idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES zyrthi-hal-adapter-esp32
    PRIV_REQUIRES zyrthi-hal
)
