add_subdirectory(config)
add_subdirectory(drivers)
add_subdirectory(cars)
add_subdirectory(data)
add_subdirectory(tracks)

install(FILES user-files DESTINATION ${datadir})
