# CMakeList.txt: проект CMake для TcpServer; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#

# Добавьте источник в исполняемый файл этого проекта.
add_executable (TcpServer 
	"main.cpp"
	"utils/args/InputArgs.cpp"
	"utils/args/ArgsHandler.cpp"
	"utils/args/CommandInfo.cpp"
	"console/console.cpp"
	"service/service.cpp"
	"net/TcpServer.cpp" 
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TcpServer PROPERTY CXX_STANDARD 20)
endif()

# TODO: Добавьте тесты и целевые объекты, если это необходимо.
