add_subdirectory("misc")
add_subdirectory("T1005")
add_subdirectory("T1057")
add_subdirectory("T1059.001")
add_subdirectory("T1070.004")
add_subdirectory("T1082")
add_subdirectory("T1083")
add_subdirectory("T1547.001")


SET(SOURCES
    main.c
	runtime_linking.c
	runtime_linking.h
	penter.asm
)

target_sources(${EXECUTABLE} PRIVATE ${SOURCES})