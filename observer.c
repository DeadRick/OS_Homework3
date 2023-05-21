#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    int socketId;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    // Создание сокета
    if ((socketId = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    // Преобразование IP-адреса из текстового в бинарный формат
    if (inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr)) <= 0)
    {
        perror("Ошибка преобразования адреса");
        exit(EXIT_FAILURE);
    }

    // Подключение к серверу
    if (connect(socketId, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Ошибка подключения");
        exit(EXIT_FAILURE);
    }

    // Отправка типа клиента (наблюдатель)
    int clientType = 2;
    send(socketId, &clientType, sizeof(clientType), 0);

    printf("Наблюдатель: успешно подключен\n");

    // Цикл получения информации от сервера
    while (1)
    {
        // Получение стоимости склада от сервера
        int receivedBytes = recv(socketId, buffer, BUFFER_SIZE, 0);
        if (receivedBytes <= 0)
            break;

        // Преобразование стоимости в число
        int warehouseValue;
        sscanf(buffer, "%d", &warehouseValue);

        // Вывод информации о стоимости склада
        printf("Наблюдатель: текущая стоимость склада: %d$\n", warehouseValue);
    }

    // Закрытие сокета
    close(socketId);

    return 0;
}
