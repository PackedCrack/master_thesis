SET(SOURCES
	common.c
	common.h
	vector.c
	vector.h
	str.c
	str.h
)

target_sources(${EXECUTABLE} PRIVATE ${SOURCES})