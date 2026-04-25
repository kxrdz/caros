# User settings.
install(FILES
   downloadservers.xml
   graph.xml
   language.xml
   logging.xml
   networking.xml
   osghudconfig.xml
   raceengine.xml
   screen.xml
   sound.xml
   webserver.xml
   DESTINATION ${datadir}/config
)

add_subdirectory(raceman)
