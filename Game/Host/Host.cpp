#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define PORT "8080"
#define BUFLEN 512

#include <iostream>
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include <iomanip> // Для корректного вывода таблицы

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

WSADATA wsaData;
SOCKET ListenSocket = INVALID_SOCKET, ClientSocket1 = INVALID_SOCKET, ClientSocket2 = INVALID_SOCKET;
struct addrinfo* result = NULL, hints;
char board[3][3] = { {' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '} };
char currentPlayer = 'X';
bool gameOver = false;

// Вывод игрового поля
void printBoard(char board[3][3], SOCKET clientSocket, char turn, bool isYourTurn) {
    string boardMessage = "\nКрестики-нолики\n";
    boardMessage += "Ваш символ: " + string(1, turn) + "\n\n";

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            boardMessage += " ";
            boardMessage += board[i][j];
            boardMessage += " ";
            if (j < 2) boardMessage += "|";
        }
        boardMessage += "\n";
        if (i < 2) boardMessage += "---|---|---\n";
    }

    if (isYourTurn) {
        boardMessage += "\nВаш ход! Введите координаты (1-9): ";
    }
    else {
        boardMessage += "\nСейчас не ваш ход. Ожидайте...\n";
    }

    send(clientSocket, boardMessage.c_str(), boardMessage.length(), 0);
}

// Проверка на победу
bool checkWin(char board[3][3], char player) {
    for (int i = 0; i < 3; i++)
        if (board[i][0] == player && board[i][1] == player && board[i][2] == player) return true;
    for (int i = 0; i < 3; i++)
        if (board[0][i] == player && board[1][i] == player && board[2][i] == player) return true;
    if (board[0][0] == player && board[1][1] == player && board[2][2] == player) return true;
    if (board[0][2] == player && board[1][1] == player && board[2][0] == player) return true;
    return false;
}

// Проверка на ничью
bool isBoardFull(char board[3][3]) {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (board[i][j] == ' ') return false;
    return true;
}

// Поток игрока
DWORD WINAPI PlayerThread(LPVOID lpParam) {
    int playerIndex = *(int*)lpParam;
    SOCKET clientSocket = (playerIndex == 0) ? ClientSocket1 : ClientSocket2;
    SOCKET otherSocket = (playerIndex == 0) ? ClientSocket2 : ClientSocket1;
    char playerSymbol = (playerIndex == 0) ? 'X' : 'O';
    char recvbuf[BUFLEN];

    // Отправляем начальное поле
    printBoard(board, clientSocket, playerSymbol, (playerIndex == 0));

    while (!gameOver) {
        // Получаем ход от игрока
        ZeroMemory(recvbuf, BUFLEN);
        int iResult = recv(clientSocket, recvbuf, BUFLEN, 0);
        if (iResult <= 0) break;

        int move = atoi(recvbuf);
        int row = (move - 1) / 3;
        int col = (move - 1) % 3;

        if (move < 1 || move > 9 || board[row][col] != ' ') {
            send(clientSocket, "Некорректный ход. Попробуйте снова.\n", 40, 0);
            continue;
        }

        // Обновляем поле
        board[row][col] = playerSymbol;

        // Отправляем обновленное поле всем игрокам
        printBoard(board, clientSocket, playerSymbol, false);
        printBoard(board, otherSocket, playerSymbol, true);

        // Проверка на победу
        if (checkWin(board, playerSymbol)) {
            printBoard(board, clientSocket, playerSymbol, true);
            send(clientSocket, "Вы победили!\n", 15, 0);
            send(otherSocket, "Противник победил!\n", 20, 0);
            gameOver = true;
            break;
        }

        // Проверка на ничью
        if (isBoardFull(board)) {
            send(ClientSocket1, "Ничья!\n", 10, 0);
            send(ClientSocket2, "Ничья!\n", 10, 0);
            gameOver = true;
            break;
        }

        // Сменить ход
        currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    }

    closesocket(clientSocket);
    return 0;
}

int main() {
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PORT, &hints, &result);
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);
    listen(ListenSocket, SOMAXCONN);

    // Ожидаем подключение игроков
    ClientSocket1 = accept(ListenSocket, NULL, NULL);
    ClientSocket2 = accept(ListenSocket, NULL, NULL);

    int playerIndex1 = 0, playerIndex2 = 1;
    CreateThread(NULL, 0, PlayerThread, &playerIndex1, 0, NULL);
    CreateThread(NULL, 0, PlayerThread, &playerIndex2, 0, NULL);

    Sleep(INFINITE);

    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}
