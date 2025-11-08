# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g

# Цели по умолчанию
all: tcpclient tcpserver udpclient udpserver

# TCP клиент
tcpclient: tcpclient.c
	$(CC) $(CFLAGS) -o tcpclient tcpclient.c

# TCP сервер
tcpserver: tcpserver.c
	$(CC) $(CFLAGS) -o tcpserver tcpserver.c

# UDP клиент
udpclient: udpclient.c
	$(CC) $(CFLAGS) -o udpclient udpclient.c

# UDP сервер
udpserver: udpserver.c
	$(CC) $(CFLAGS) -o udpserver udpserver.c

# Запуск TCP теста на свободном порту
run-tcp: tcpclient tcpserver
	@echo "Starting TCP server on port 8082..."
	./tcpserver 8082 100 &
	@sleep 2
	@echo "Starting TCP client..."
	@echo "Hello TCP Server" | ./tcpclient 127.0.0.1 8082 100
	@sleep 1
	@pkill -f "tcpserver" || true

# Запуск UDP теста на свободном порту
run-udp: udpclient udpserver
	@echo "Starting UDP server on port 8083..."
	./udpserver 8083 1024 &
	@sleep 2
	@echo "Starting UDP client..."
	@echo "Hello UDP Server" | ./udpclient 127.0.0.1 8083 1024
	@sleep 1
	@pkill -f "udpserver" || true

# Остановка всех серверов
stop-servers:
	@pkill -f "tcpserver" || true
	@pkill -f "udpserver" || true
	@echo "Servers stopped"

# Очистка
clean:
	rm -f tcpclient tcpserver udpclient udpserver

# Проверка занятых портов
check-ports:
	@echo "Checking common ports..."
	@for port in 8080 8081 8082 8083; do \
		if netstat -tuln 2>/dev/null | grep :$$port > /dev/null; then \
			echo "Port $$port: OCCUPIED"; \
		else \
			echo "Port $$port: FREE"; \
		fi; \
	done

.PHONY: all run-tcp run-udp stop-servers clean check-ports