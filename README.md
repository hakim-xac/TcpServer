TCP-сервер на C++ (Windows) с двумя режимами работы — как служба Windows и как консольный демон.\
Для работы в качестве службы Windows, требуются права администратора.

Доступные команды:
```bash
D:\PROJECTS\CPP\TcpServer\out\build\x64-release\TcpServer>TcpServer.exe 
TcpServer.exe < command > TcpServer.exe [ options ]
Commands:
  1)--daemon - Runs the program as a daemon
  2)--stop-service - Stops the service
  3)--start-service - Starts the service
  4)--install-service - Installs the service into the system
  5)--uninstall-service - Removes the service from the system
```
Работает в паре с https://github.com/hakim-xac/TcpClient - GUI клиент.
