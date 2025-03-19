#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define PORT "8080"
#define BUFLEN 512
#define SPAM_TIMEOUT 1

#include <iostream>
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

WSADATA wsaData;
SOCKET ListenSocket = INVALID_SOCKET, ClientSocket = INVALID_SOCKET;
struct addrinfo* result = NULL, * ptr = NULL, hints;
string clientName;
char recvbuf[BUFLEN];
int iResult;

void printBoard(char board[3][3]) {
    system("cls");
    cout << "Крестики-нолики" << endl;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            cout << " " << board[i][j] << " ";
            if (j < 2) cout << "|";
        }
        cout << endl;
        if (i < 2) cout << "---|---|---" << endl;
    }
}

bool checkWin(char board[3][3], char player) {
    // Проверка строк
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == player && board[i][1] == player && board[i][2] == player) return true;
    }
    // Проверка столбцов
    for (int i = 0; i < 3; i++) {
        if (board[0][i] == player && board[1][i] == player && board[2][i] == player) return true;
    }
    // Проверка диагоналей
    if (board[0][0] == player && board[1][1] == player && board[2][2] == player) return true;
    if (board[0][2] == player && board[1][1] == player && board[2][0] == player) return true;

    return false;
}

bool isBoardFull(char board[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == ' ') return false;
        }
    }
    return true;
}

int main() {
    setlocale(0, "rus");
    // Инициализация Winsock

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "Ошибка инициализации Winsock\n";
        return 1;
    }

    // Получение локального IP
    char hostName[256];
    if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR) {
        cout << "Ошибка получения имени хоста\n";
        WSACleanup();
        return 2;
    }

    // Вывод информации
    cout << "Вы играете в Крестики-Нолики!\n";

    // Запрос имени
    cout << "Введите ваше имя: ";
    getline(cin, clientName);

    // Создание сокета для прослушивания
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &result) != 0) {
        cout << "Ошибка получения адреса\n";
        WSACleanup();
        return 2;
    }

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        cout << "Ошибка создания сокета\n";
        WSACleanup();
        return 3;
    }

    if (bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        cout << "Ошибка привязки\n";
        closesocket(ListenSocket);
        WSACleanup();
        return 4;
    }

    freeaddrinfo(result);

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Ошибка прослушивания\n";
        closesocket(ListenSocket);
        WSACleanup();
        return 5;
    }

    cout << "Ожидаем подключения клиента...\n";

    // Ожидание подключения клиента
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        cout << "Ошибка accept: " << WSAGetLastError() << "\n";
        closesocket(ListenSocket);
        WSACleanup();
        return 6;
    }

    cout << "Клиент подключился. Начинаем игру!\n";

    char board[3][3] = { {' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '} };
    bool gameOver = false;
    char currentPlayer = 'X';

    while (!gameOver) {
        printBoard(board);

        if (currentPlayer == 'X') {
            cout << "Ваш ход, " << clientName << " (X): ";
        }
        else {
            cout << "Ход противника (O): ";
        }

        int move;
        cin >> move;

        // Проверка на корректность хода
        int row = (move - 1) / 3;
        int col = (move - 1) % 3;

        if (move < 1 || move > 9 || board[row][col] != ' ') {
            cout << "Некорректный ход. Попробуйте снова.\n";
            continue;
        }

        // Обновляем игровое поле
        board[row][col] = currentPlayer;

        // Отправляем ход другому игроку
        string moveMessage = "Ход: " + to_string(move);
        send(ClientSocket, moveMessage.c_str(), moveMessage.length(), 0);

        // Проверяем победителя
        if (checkWin(board, currentPlayer)) {
            printBoard(board);
            if (currentPlayer == 'X') {
                cout << clientName << " победил!\n";
            }
            else {
                cout << "Противник победил!\n";
            }
            gameOver = true;
        }
        else if (isBoardFull(board)) {
            printBoard(board);
            cout << "Ничья!\n";
            gameOver = true;
        }

        // Меняем игрока
        currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    }

    closesocket(ClientSocket);
    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}
