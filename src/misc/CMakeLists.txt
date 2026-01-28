SET(SOURCES
	common.c
	common.h
	vector.c
	vector.h
	str.c
	str.h
	wstr.c
	wstr.h
)

target_sources(${EXECUTABLE} PRIVATE ${SOURCES})