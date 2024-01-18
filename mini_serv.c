#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

char *tmp = NULL;

void	ft_error()
{
	char *err = "Fatal error\n";
	write(2, err, strlen(err));
	exit(1);
}

// Since we are asked to write "client %d: " before each line, we need to extract every potential line one by one

int extract_message(char *str)
{
	int i = 0;

	if (!strlen(str)) // If nothing left to read, we stop sending anything
		return 0;

	while(str[i] && str[i] != '\n')
		i++;

	tmp = malloc(sizeof(char) * (i + 2));
	if (!tmp)
		ft_error();

	i = 0;

	while (str[i] && str[i] != '\n')
	{
		tmp[i] = str[i];
		i++;
	}

	strcat(tmp, "\n\0"); // No need to send the \n to everyone with another line in the main loop

	return 1;
}

void	sendAll(int fd, char *str, fd_set *writefds, int fdMax)
{
	int len = strlen(str);

	for (int i = 0; i <= fdMax; i++)
	{
		if (FD_ISSET(i, writefds) && i != fd) // We don't want to send the message to its sender
			send(i, str, len, 0);
	}
}

int	initsock(fd_set *active)
{
	int reuse = 1;
	int fdMax = socket(AF_INET, SOCK_STREAM, 0);
	if (fdMax < 0)
		ft_error();
	FD_SET(fdMax, active);
	return fdMax;
}

int main(int ac, char **argv)
{
	if (ac != 2)
	{
		char *err = "Wrong number of arguments\n";
		write(2, err, strlen(err));
		exit(1);
	}

	fd_set readfds, writefds, active;
	FD_ZERO(&active);
	char buf_read[1000], buf_write[40];
	int sockfd = initsock(&active);
	int idNext = 0, fdMax = sockfd, offset, client[65536], res;

	struct sockaddr_in servaddr = {0}; // No need of bzero(servaddr, sizeof(servaddr))
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	servaddr.sin_port = htons(atoi(argv[1]));

	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))))
		ft_error();
	if (listen(sockfd, SOMAXCONN))
		ft_error();
	while (1)
	{
		readfds = writefds = active;
		if (select(fdMax + 1, &readfds, &writefds, NULL, NULL) < 0)
			ft_error();
		for (int fd = 0; fd <= fdMax; fd++)
		{
			if (!FD_ISSET(fd, &readfds))
				continue;
			if (fd == sockfd)
			{
				int newFd = accept(sockfd, NULL, NULL);
				if (newFd <= 0)
					ft_error();
				fdMax = newFd > fdMax ? newFd : fdMax;
				client[newFd] = idNext++;
				FD_SET(newFd, &active);
				sprintf(buf_write, "server: client %d just arrived\n", client[newFd]);
				sendAll(newFd, buf_write, &writefds, fdMax);
			}
			else
			{
				read = recv(fd, buf_read, 999, 0);
				if (read <= 0)
				{
					sprintf(buf_write, "server: client %d just left\n", client[fd]);
					sendAll(fd, buf_write, &writefds, fdMax);
					FD_CLR(fd, &active);
					close(fd);
				}

				buf_read[read] = '\0'; // If the message is longer than 999 characters
				offset = 0;

				while (extract_message(&buf_read[offset])) // Start the reading from when we last ended it
				{
					sprintf(buf_write, "client %d: ", client[fd]);
					sendAll(fd, buf_write, &writefds, fdMax);
					sendAll(fd, tmp, &writefds, fdMax);
					offset += strlen(tmp); // Get the end of what we already read
					free(tmp);
				}
			}

		}
	}
	return 0;
}
