#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define PORT "8080"
#define BUFLEN 512

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

WSADATA wsaData;
SOCKET ConnectSocket = INVALID_SOCKET;
struct addrinfo* result = NULL, * ptr = NULL, hints;
char recvbuf[BUFLEN];
int iResult;

int main() {
    setlocale(0, "rus");

    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "Ошибка инициализации Winsock\n";
        return 1;
    }

    // Запрос IP сервера
    string serverIp;
    cout << "Введите IP адрес сервера: ";
    cin >> serverIp;

    // Настройки соединения
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(serverIp.c_str(), PORT, &hints, &result) != 0) {
        cout << "Ошибка получения адреса\n";
        WSACleanup();
        return 2;
    }

    // Создание сокета
    ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
        cout << "Ошибка создания сокета\n";
        WSACleanup();
        return 3;
    }

    // Подключение к серверу
    iResult = connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        cout << "Ошибка подключения к серверу\n";
        closesocket(ConnectSocket);
        WSACleanup();
        return 4;
    }

    freeaddrinfo(result);

    cout << "Подключение к серверу успешно!\n";

    bool gameOver = false;

    while (!gameOver) {
        // Получение данных от сервера
        iResult = recv(ConnectSocket, recvbuf, BUFLEN - 1, 0);
        if (iResult <= 0) break;

        recvbuf[iResult] = '\0';  // Устанавливаем конец строки
        system("cls"); // Очистка экрана перед выводом нового поля
        cout << recvbuf << endl;

        // Если сервер ждет ввода - вводим координаты
        if (strstr(recvbuf, "Введите координаты") != NULL) {
            int move;
            cout << "Ваш ход: ";
            cin >> move;

            string moveStr = to_string(move);
            send(ConnectSocket, moveStr.c_str(), moveStr.length(), 0);
        }
    }

    // Закрываем соединение
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
