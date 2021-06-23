#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <mysqlx/xdevapi.h>
#include<Windows.h>
#include <ctime>
#include <Winsock2.h>
//#include <Ws2tcpip.h>
//#include <stdio.h>
#include <iostream>
#include<fstream>


#define WM_SOCKET (WM_USER + 1)
#define DEFAULT_PORT 60001
#define MAX_QUEUE 100

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

//using namespace std;
mysqlx::Table* Employees;
mysqlx::Table* Inactions;
struct worktime
{
	int Startwork;
	int Endwork;
	int Startlunch;
	int Endlunch;
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int main(int argc, char** argv)
{
	mysqlx::Session mySession("localhost", 33060, "user", "Pass");
	mysqlx::Schema db = mySession.getSchema("mydb");
	mysqlx::Table employeesMain = db.getTable("Employees");
	mysqlx::Table inactionMain = db.getTable("Inactions");
	Employees = &employeesMain;
	Inactions = &inactionMain;

	//---

	MSG msg;
	int ret;
	int iResult;
	WSADATA wsaData;
	SOCKET Listen;
	SOCKADDR_IN InternetAddr;

	InternetAddr.sin_family = AF_INET;
	InternetAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	InternetAddr.sin_port = htons(DEFAULT_PORT);

	WNDCLASS wndclass = { };
	const wchar_t CLASS_NAME[] = L"AsyncSelect";

	wndclass.lpfnWndProc = WindowProc; // Это указатель на определяемую приложением функцию, называемую оконной процедурой
	wndclass.hInstance = nullptr; //  Это дескриптор экземпляра приложения. Это значение получено из параметра hInstance wWinMain
	wndclass.lpszClassName = CLASS_NAME; // Это строка, определяющая класс окна

	if (RegisterClass(&wndclass) == 0)
	{
		std::cout << "RegisterClass() failed with error" << GetLastError() << std::endl;
		return NULL;
	}
	else
		std::cout << "RegisterClass() is OK!" << std::endl;

	// Создание окна
	HWND Window = CreateWindowEx(
		0,                              // Optional window styles.
		CLASS_NAME,                     // Window class
		L"Server",    // Window text
		WS_OVERLAPPEDWINDOW,            // Window style

		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		NULL,       // Parent window    
		NULL,       // Menu
		NULL,  // Instance handle
		NULL        // Additional application data
	);

	if (Window == NULL)
	{
		return 0;
	}

	//ShowWindow(Window, nCmdShow);

	// Подготовка эхо-сервера
	iResult = WSAStartup((2, 2), &wsaData);
	if (iResult != 0)
	{
		std::cout << "WSAStartup() failed with error" << WSAGetLastError() << std::endl;
		return 1;
	}
	else
		std::cout << "WSAStartup() is OK!" << std::endl;

	Listen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Listen == INVALID_SOCKET)
	{
		std::cout << "socket() failed with error" << WSAGetLastError() << std::endl;
		return 1;
	}
	else
		std::cout << "socket() is looks fine!" << std::endl;

	iResult = bind(Listen, (SOCKADDR*)&InternetAddr, sizeof(InternetAddr));
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "bind() failed with error" << WSAGetLastError() << std::endl;
		return 1;
	}
	else
		std::cout << "bind() is OK!" << std::endl;

	if (WSAAsyncSelect(Listen, Window, WM_SOCKET, FD_ACCEPT | FD_CLOSE) == 0)
		std::cout << "WSAAsyncSelect() is OK!" << std::endl;
	else
		std::cout << "WSAAsyncSelect() failed with error code" << WSAGetLastError() << std::endl;

	if (listen(Listen, MAX_QUEUE))
	{
		std::cout << "listen() failed with error" << WSAGetLastError() << std::endl;
		return 1;
	}
	else
		std::cout << "listen() is also OK! I am listening now..." << std::endl;

	// Перевод и отправка сообщений окна потока приложения
	while (ret = GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (closesocket(Listen) == SOCKET_ERROR)
	{
		//wprintf(L"closesocket failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	WSACleanup();

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SOCKET Accept;
	TCHAR recvbuf[1025];
	int recvbuflen = 1024 * sizeof(TCHAR);
	int iResult = -1;

	if (uMsg == WM_SOCKET)
	{
		if (WSAGETSELECTERROR(lParam))
		{
			std::cout << "Socket failed with error" << WSAGETSELECTERROR(lParam) << std::endl;
			closesocket(wParam);
		}
		else
		{
			std::cout << "Socket looks fine!" << std::endl;
			switch (WSAGETSELECTEVENT(lParam))
			{
			case FD_ACCEPT:
				if ((Accept = accept(wParam, NULL, NULL)) == INVALID_SOCKET)
				{
					std::cout << "accept() failed with error" << WSAGetLastError() << std::endl;
					break;
				}
				else
					std::cout << "accept() is OK!" << std::endl;
				WSAAsyncSelect(Accept, hwnd, WM_SOCKET, FD_READ | FD_WRITE | FD_CLOSE);
				break;

			case FD_READ:
				iResult = recv(wParam, (char*)recvbuf, recvbuflen, 0);
				if (iResult > 0) 
				{
					std::cout << "Bytes received: " << iResult << std::endl;
					recvbuf[iResult / sizeof(TCHAR)] = L'\0';

					std::wstring package(recvbuf);
					std::string::size_type start_username = package.find(L"Username:") + 9;
					std::string::size_type end_username = package.find(L";Type:");
					std::string::size_type start_msg = package.find(L";Msg:") + 5;
					std::string::size_type end_msg = package.rfind(L";");

					std::wstring username = package.substr(start_username, end_username - start_username); // Вывод имя пользователя
					std::wstring typemsg = package.substr(end_username + 6, 4); //Поиск типа сообщния (длина типа сообщения - всегда 4 символа)
					std::wstring packagemsg = package.substr(start_msg, end_msg - start_msg); //Поиск сообщния (содержание)

					if (!typemsg.compare(L"HRTB"))
					{
						std::cout << "Heartbeat" << std::endl;
					}
					else if (!typemsg.compare(L"AFKB"))
					{
						mysqlx::Value value;
						int id;
						std::string rawtime;
						worktime employee;

						mysqlx::RowResult res = Employees->select("*")
							.where("username like :param")
							.bind("param", username).execute();						

						if (res.count() == 0)
						{
							Employees->insert("username").values(username).execute();
							res = Employees->select("*")
								.where("username like :param")
								.bind("param", username).execute();
						}

						mysqlx::Row row = res.fetchOne();

						id = (int)row[0];
						bool nohastime = false;

						if (row[3].isNull())
							nohastime = true;
						else
						{
							rawtime = (std::string)row[3];
							employee.Startwork = (int)rawtime[1] * 100 + (int)rawtime[0];
						}
						if (row[4].isNull())
							nohastime = true;
						else
						{
							rawtime = (std::string)row[4];
							employee.Endwork = (int)rawtime[1] * 100 + (int)rawtime[0];
						}
						if(row[5].isNull())
							nohastime = true;
						else
						{
							rawtime = (std::string)row[5];
							employee.Startlunch = (int)rawtime[1] * 100 + (int)rawtime[0];
						}
						if (row[6].isNull())
							nohastime = true;
						else
						{
							rawtime = (std::string)row[6];
							employee.Endlunch = (int)rawtime[1] * 100 + (int)rawtime[0];
						}

						time_t rawtime_t;
						struct tm timeinfo;
						time(&rawtime_t);
						localtime_s(&timeinfo, &rawtime_t);
						int localtimeHourMin = timeinfo.tm_hour * 100 + timeinfo.tm_min;

						if (!nohastime && localtimeHourMin >= employee.Startwork && localtimeHourMin < employee.Endwork)
						{
							if (!(localtimeHourMin >= employee.Startlunch && localtimeHourMin <= employee.Endlunch))
							{
								std::cout << "AFK\n";
								Inactions->insert("idEmployees").values(id).execute();
							}
							else
								std::cout << "Lunch\n";
						}
						else if(nohastime)
						{
							Inactions->insert("idEmployees").values(id).execute();
						}
						else
							std::cout << "Relax\n";
					}
				}
				else if (iResult == 0)
					std::cout << "Connection closing...\n";
				else 
				{
					std::cout << "recv failed with error: "<< WSAGetLastError() << std::endl;
					closesocket(wParam);
				}

				break;

			case FD_WRITE:
				break;

			case FD_CLOSE:
				std::cout << "Closing socket" << wParam << std::endl;
				closesocket(wParam);
				break;
			}
		}
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}