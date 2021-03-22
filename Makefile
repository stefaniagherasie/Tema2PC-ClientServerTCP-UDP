CFLAGS = -Wall -g


all: server subscriber

# Compileaza server.c
server: server.cpp

# Compileaza client.c
subscriber: subscriber.cpp

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_subscriber:
	./subscriber ${CLIENT_ID} ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
