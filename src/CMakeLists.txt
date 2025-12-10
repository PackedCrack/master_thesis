SET(SOURCES
    main.c
	common.c
	common.h
	vector.c
	vector.h
	str.c
	str.h
	runtime_linking.c
	runtime_linking.h
)

target_sources(${EXECUTABLE} PRIVATE ${SOURCES})