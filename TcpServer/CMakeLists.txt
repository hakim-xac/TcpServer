# CMakeList.txt: проект CMake для TcpServer; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#

# Добавьте источник в исполняемый файл этого проекта.
add_executable (TcpServer 
	#"TcpServer.cpp"
	"main.cpp"
	"utils/args/InputArgs.h"
	"utils/args/InputArgs.cpp"
	"utils/args/ArgsHandler.h"
	"utils/args/ArgsHandler.cpp"
	"utils/args/CommandInfo.h"
	"utils/args/CommandInfo.cpp"
	"utils/logger/LoggerTags.h"
	"utils/logger/LoggerBaseInterface.h"
	"utils/logger/logger.h"
	"utils/args/args.h"
	"routes/routes.h"
	"console/console.h"
	"console/console.cpp"
	"service/service.h" 
	"service/service.cpp"
	 "utils/helper.h"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TcpServer PROPERTY CXX_STANDARD 20)
endif()

# TODO: Добавьте тесты и целевые объекты, если это необходимо.
