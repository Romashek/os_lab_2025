# Компилятор и флаги
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -I.
LDFLAGS = -lpthread

# Цели
all: client server1 server2

# Библиотека общих функций
common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c -o common.o

# Клиент
client: client.c common.o
	$(CC) $(CFLAGS) -o client client.c common.o

# Сервер 1
server1: server.c common.o
	$(CC) $(CFLAGS) -o server1 server.c common.o $(LDFLAGS)

# Сервер 2  
server2: server.c common.o
	$(CC) $(CFLAGS) -o server2 server.c common.o $(LDFLAGS)

# Запуск серверов
run-servers: server1 server2
	./server1 --port 20001 --tnum 2 & \
	./server2 --port 20002 --tnum 2 & \
	echo "Servers started on ports 20001 and 20002"

# Запуск клиента
run-client: client
	./client --k 20 --mod 1000000007 --servers servers.txt

# Создание файла servers.txt
setup:
	echo "127.0.0.1:20001" > servers.txt
	echo "127.0.0.1:20002" >> servers.txt

# Полный тест
test: setup run-servers
	sleep 2
	./client --k 20 --mod 1000000007 --servers servers.txt

# Остановка серверов
stop-servers:
	@pkill -f "server[12]" || true
	@echo "Servers stopped"

# Очистка
clean:
	rm -f client server1 server2 common.o servers.txt

.PHONY: all run-servers run-client setup test stop-servers clean