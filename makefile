CC = gcc

make: webserver.c client.c
	@$(CC) -o webserver webserver.c 
	@$(CC) -o client client.c

.PHONY: clean
clean:
	@rm -f webserver
	@rm -f client
	@rm -f client.html
	@rm -f client.gif
	@rm -f client_error.html
