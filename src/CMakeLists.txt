SET(SOURCES
    main.c
	vector.c
	vector.h
)

target_sources(${EXECUTABLE} PRIVATE ${SOURCES})