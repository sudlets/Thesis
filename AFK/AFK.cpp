// AFK.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Windows.h>
#include <winuser.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>
#include <iostream>
#include <fstream>


#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")
//#pragma comment (lib, "AdvApi32.lib")

typedef int(__stdcall* MSGBOXAAPI)(IN HWND hWnd,
    IN LPCSTR lpText, IN LPCSTR lpCaption,
    IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
typedef int(__stdcall* MSGBOXWAPI)(IN HWND hWnd,
    IN LPCWSTR lpText, IN LPCWSTR lpCaption,
    IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);

int MessageBoxTimeoutA(IN HWND hWnd, IN LPCSTR lpText,
    IN LPCSTR lpCaption, IN UINT uType,
    IN WORD wLanguageId, IN DWORD dwMilliseconds);
int MessageBoxTimeoutW(IN HWND hWnd, IN LPCWSTR lpText,
    IN LPCWSTR lpCaption, IN UINT uType,
    IN WORD wLanguageId, IN DWORD dwMilliseconds);

#ifdef UNICODE
#define MessageBoxTimeout MessageBoxTimeoutW
#else
#define MessageBoxTimeout MessageBoxTimeoutA
#endif 

#define INFO_BUFFER_SIZE 64
#define DEFAULT_BUFLEN 512
#define MAX_TIME_AFK 60 // Время отстутствия в секундах  2700
#define TIME_AFK_WARNING 30 // Время отстутствия в секундах для предупреждения 1800
#define WAIT_MASSAGEBOX (MAX_TIME_AFK - TIME_AFK_WARNING)
#define AFK_INTERVAL (MAX_TIME_AFK / 10)
#define HEARTBEAT_INTERVAL (MAX_TIME_AFK / 2)
#define TIME_SLEEP (AFK_INTERVAL / 3)



SOCKET ConnectSocket = INVALID_SOCKET;
HANDLE Thread_Inaction_algorithm = nullptr;
HANDLE Thread_MessageBox = nullptr;
std::string IPaddress;
USHORT Port;
struct sockaddr_in saServer;

DWORD WINAPI Inaction_algorithm(LPVOID lpParam);
LRESULT CALLBACK WindowProc(_In_ HWND, _In_ UINT, _In_ WPARAM, _In_ LPARAM);
DWORD WINAPI displaybox(LPVOID lpParam);
bool connect_to_server(void);

INT WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR lpCmdLine, _In_ INT nCmdShow)
{
    std::ifstream fin("IPaddress.txt");
    if (fin.is_open())
    {
        fin >> IPaddress;
        fin >> Port;
        fin.close();
    }
    else
    {
        return 2;
    }

    WSADATA wsaData;
    int iResult;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&saServer, sizeof(saServer));
    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = inet_addr(IPaddress.c_str());
    saServer.sin_port = htons(Port);

    while (!connect_to_server())
        Sleep(300000);
    
    // Register the window class.
    const wchar_t CLASS_NAME[] = L"Sample Window Class";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc; // Это указатель на определяемую приложением функцию, называемую оконной процедурой
    wc.hInstance = hInstance; //  Это дескриптор экземпляра приложения. Это значение получено из параметра hInstance wWinMain
    wc.lpszClassName = CLASS_NAME; // Это строка, определяющая класс окна

    RegisterClass(&wc); // Эта функция регистрирует класс окна в операционной системе

    // Создание окна
    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Learn to Program Windows",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL)
    {
        return 3;
    }

    //ShowWindow(hwnd, nCmdShow);

    Thread_Inaction_algorithm = CreateThread(
        nullptr, 
        0, 
        Inaction_algorithm, 
        nullptr, 
        0, 
        nullptr); // создание потока для функции алгоритма бездействия

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
 
    if (Thread_Inaction_algorithm != nullptr)
        TerminateThread(Thread_Inaction_algorithm, 0);

    CloseHandle(Thread_Inaction_algorithm);

    // cleanup
    if (closesocket(ConnectSocket) == SOCKET_ERROR)
    {
        //wprintf(L"closesocket failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    WSACleanup();
    return 0;
}

DWORD WINAPI Inaction_algorithm(LPVOID lpParam)
{
    LASTINPUTINFO* StructLastInputInfo = new LASTINPUTINFO;
    StructLastInputInfo->cbSize = sizeof(PLASTINPUTINFO);
    ULONGLONG current_time; // Текущее системное время
    ULONGLONG current_afk_time; // Текущее время бездействия пользователя
    ULONGLONG next_heartbeat_time; // Время следующего сердцебиения
    bool access = false;
    bool heartbeat = false;
    int iResult;

    TCHAR  infoBuf[INFO_BUFFER_SIZE];
    DWORD  bufCharCount = INFO_BUFFER_SIZE;
    GetUserName(infoBuf, &bufCharCount);
    std::wstring msgHeartbeat = L"Username:";
    msgHeartbeat.append(infoBuf);
    msgHeartbeat.append(L";Type:HRTB;Msg:Heartbeat;");
    std::wstring msgAFK = L"Username:";
    msgAFK.append(infoBuf);
    msgAFK.append(L";Type:AFKB;Msg:Away from keyboard;");

    size_t send_hrtb_size = msgHeartbeat.size();
    const TCHAR* send_hrtb_buf = new TCHAR[send_hrtb_size];
    send_hrtb_buf = msgHeartbeat.c_str();

    size_t send_afkb_size = msgAFK.size();
    const TCHAR* send_afkb_buf = new TCHAR[send_afkb_size];
    send_afkb_buf = msgAFK.c_str();

    next_heartbeat_time = GetTickCount64() / 1000 + HEARTBEAT_INTERVAL;

    while (true)
    {
        GetLastInputInfo(StructLastInputInfo);
        current_time = GetTickCount64() / 1000;
        current_afk_time = (GetTickCount64() - StructLastInputInfo->dwTime) / 1000;

        if (access && current_afk_time >= MAX_TIME_AFK && current_afk_time % MAX_TIME_AFK < AFK_INTERVAL)
        {
            do
            {
                iResult = send(ConnectSocket, (char*)send_afkb_buf, wcslen(send_afkb_buf) * sizeof(TCHAR), 0); // Добавить проверку на ошибку +
                if (iResult == SOCKET_ERROR)
                {
                    while (!connect_to_server())
                        Sleep(300000);
                }
            } while (iResult == SOCKET_ERROR);
        }

        if (heartbeat)
        {
            heartbeat = false;
            next_heartbeat_time = current_time + HEARTBEAT_INTERVAL;
            do
            {
                iResult = send(ConnectSocket, (char*)send_hrtb_buf, wcslen(send_hrtb_buf) * sizeof(TCHAR), 0); // Добавить проверку на ошибку +
                if (iResult == SOCKET_ERROR)
                {
                    while (!connect_to_server())
                        Sleep(300000);
                }
            } while (iResult == SOCKET_ERROR);
        }

        if (current_afk_time > TIME_AFK_WARNING)
        {
            MessageBoxTimeout(
                NULL,
                (LPCWSTR)L"Ты тут? Алло!",
                (LPCWSTR)L"Статус",
                MB_SYSTEMMODAL | MB_ICONINFORMATION | MB_OK,
                0,
                30000
            );
        }

        if (!access)
            access = current_afk_time % MAX_TIME_AFK > AFK_INTERVAL;

        if (!heartbeat)
            heartbeat = current_time > next_heartbeat_time;

        Sleep(1000);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    int result;
    switch (uMsg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        /*Функция PostQuitMessage помещает сообщение WM_QUIT в очередь
        сообщений. WM_QUIT - это специальное сообщение: оно заставляет
        GetMessage возвращать ноль, сигнализируя об окончании цикла
        сообщений.*/
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

DWORD WINAPI displaybox(LPVOID lpParam)
{
    MessageBox(
        NULL,
        (LPCWSTR)L"Ты тут?",
        (LPCWSTR)L"Тук-тук",
        MB_ICONINFORMATION | MB_YESNO
    );
    return 0;
}

int MessageBoxTimeoutA(HWND hWnd, LPCSTR lpText,
    LPCSTR lpCaption, UINT uType, WORD wLanguageId,
    DWORD dwMilliseconds)
{
    static MSGBOXAAPI MsgBoxTOA = NULL;

    if (!MsgBoxTOA)
    {
        HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
        if (hUser32)
        {
            MsgBoxTOA = (MSGBOXAAPI)GetProcAddress(hUser32,
                "MessageBoxTimeoutA");
            //fall through to 'if (MsgBoxTOA)...'
        }
        else
        {
            //stuff happened, add code to handle it here 
            //(possibly just call MessageBox())
            return 0;
        }
    }

    if (MsgBoxTOA)
    {
        return MsgBoxTOA(hWnd, lpText, lpCaption,
            uType, wLanguageId, dwMilliseconds);
    }

    return 0;
}

int MessageBoxTimeoutW(HWND hWnd, LPCWSTR lpText,
    LPCWSTR lpCaption, UINT uType, WORD wLanguageId, DWORD dwMilliseconds)
{
    static MSGBOXWAPI MsgBoxTOW = NULL;

    if (!MsgBoxTOW)
    {
        HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
        if (hUser32)
        {
            MsgBoxTOW = (MSGBOXWAPI)GetProcAddress(hUser32,
                "MessageBoxTimeoutW");
            //fall through to 'if (MsgBoxTOW)...'
        }
        else
        {
            //stuff happened, add code to handle it here 
            //(possibly just call MessageBox())
            return 0;
        }
    }

    if (MsgBoxTOW)
    {
        return MsgBoxTOW(hWnd, lpText, lpCaption,
            uType, wLanguageId, dwMilliseconds);
    }

    return 0;
}

bool connect_to_server(void)
{
    int iResult;

    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET)
    {
        //printf("socket failed with error: %ld\n", WSAGetLastError());
        return false;
    }

    iResult = connect(ConnectSocket, (SOCKADDR*)&saServer, sizeof(saServer));
    if (iResult == SOCKET_ERROR)
    {
        //printf("connect failed with error: %ld\n", WSAGetLastError());
        closesocket(ConnectSocket);
        return false;
    }

    return true;
}