#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#define PORT 8080

typedef struct
{
    int items;
    int value;
} StolenGoods;

typedef struct
{
    int socket;
    int isObserver;
} ClientInfo;

StolenGoods stolenGoods; // Объявление структуры StolenGoods
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Инициализация мьютекса
int observerSocket = -1; // Сокет наблюдателя
pthread_mutex_t observerMutex = PTHREAD_MUTEX_INITIALIZER; // Мьютекс для доступа к сокету наблюдателя

void *clientHandler(void *arg)
{
    ClientInfo client = *(ClientInfo *)arg;
    char buffer[1024];

    // Отправка разрешения доступа к общим ресурсам
    send(client.socket, "Access granted", strlen("Access granted"), 0);

    // Если клиент - наблюдатель, сохраняем его сокет
    if (client.isObserver)
    {
        pthread_mutex_lock(&observerMutex);
        observerSocket = client.socket;
        printf("Сервер: наблюдатель подключен\n");

        // Отправляем начальную стоимость склада
        char valueStr[20];
        snprintf(valueStr, sizeof(valueStr), "%d", stolenGoods.value);
        send(observerSocket, valueStr, strlen(valueStr), 0);
        pthread_mutex_unlock(&observerMutex);
    }

    // Главный цикл обработки клиента
    while (1)
    {
        // Получение данных от клиента
        int bytesReceived = recv(client.socket, buffer, 1024, 0);
        if (bytesReceived <= 0)
        {
            // Проблема при получении данных или соединение закрыто
            break;
        }
        buffer[bytesReceived] = '\0';

        // Проверка условия завершения
        if (stolenGoods.items <= 0 || stolenGoods.value <= 0)
        {
            printf("Сервер: все предметы похищены\n");
            break;
        }

        // Обновление структуры с похищенным имуществом
        int price = rand() % 41 + 10;
        pthread_mutex_lock(&mutex);
        stolenGoods.items--;
        stolenGoods.value -= price;

        printf("Сервер: со склада похитили товар стоимостью в %d$.\n", price);

        // Преобразование стоимости в строку
        char valueStr[20];
        snprintf(valueStr, sizeof(valueStr), "%d", stolenGoods.value);

        // Отправка стоимости клиенту
        send(client.socket, valueStr, strlen(valueStr), 0);

        // Отправка стоимости наблюдателю
        if (client.isObserver)
        {
            pthread_mutex_lock(&observerMutex);
            if (observerSocket != -1)
            {
                send(observerSocket, valueStr, strlen(valueStr), 0);
            }
            pthread_mutex_unlock(&observerMutex);
        }

        pthread_mutex_unlock(&mutex);

        // Задержка перед следующей итерацией
        sleep(1);
    }

    // Закрытие сокета клиента
    close(client.socket);
    pthread_exit(NULL);
}

int main()
{
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    char buffer[1024];
    pthread_t threadId;

    // Создание сокета
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Привязка сокета к указанному порту
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Ошибка привязки сокета");
        exit(EXIT_FAILURE);
    }

    // Ожидание подключения клиентов
    if (listen(serverSocket, 5) < 0)
    {
        perror("Ошибка ожидания подключений");
        exit(EXIT_FAILURE);
    }

    printf("Сервер: ожидание подключений...\n");

    // Инициализация структуры с похищенным имуществом
    stolenGoods.items = 100;  // Начальное количество предметов
    stolenGoods.value = 1000; // Начальная стоимость

    // Главный цикл сервера
    while (1)
    {
        // Принятие нового подключения от клиента
        if ((newSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen)) < 0)
        {
            perror("Ошибка при принятии подключения");
            exit(EXIT_FAILURE);
        }

        printf("Сервер: новое подключение принято\n");

        // Создание потока для обработки клиента
        ClientInfo client;
        client.socket = newSocket;

        // Проверка, является ли клиент наблюдателем
        
        if (strstr(buffer, "2") != NULL)
        {
            client.isObserver = 1;
        }
        else
        {
            client.isObserver = 0;
        }

        if (pthread_create(&threadId, NULL, clientHandler, (void *)&client) != 0)
        {
            perror("Ошибка при создании потока");
            exit(EXIT_FAILURE);
        }

        // Освобождение ресурсов потока
        pthread_detach(threadId);

        // Проверка условия завершения
        if (stolenGoods.items <= 0 || stolenGoods.value <= 0)
        {
            close(serverSocket);
            break;
        }
    }

    // Закрытие сокета сервера
    close(serverSocket);

    return 0;
}
