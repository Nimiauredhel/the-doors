FILE(GLOB CommonSources *.c)
idf_component_register(SRCS ${CommonSources} INCLUDE_DIRS ".")
