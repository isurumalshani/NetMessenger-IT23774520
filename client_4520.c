#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define PORT 10520

#define BUFFER_SIZE 1024
#define USERNAME_SIZE 50
#define FILENAME_SIZE 256

int sock;
volatile int running = 1;

/* --------------------------------------------------------- */
/* Send all bytes                                            */
/* --------------------------------------------------------- */

int send_all(int fd, const void *data, size_t length)
{
    size_t total = 0;
    const char *ptr = (const char *)data;

    while (total < length)
    {
        ssize_t n = send(fd,
                         ptr + total,
                         length - total,
                         0);

        if (n <= 0)
        {
            return -1;
        }

        total += (size_t)n;
    }

    return 0;
}

/* --------------------------------------------------------- */
/* Receive messages from server                              */
/* --------------------------------------------------------- */

void *receive_messages(void *arg)
{
    (void)arg;

    char buffer[BUFFER_SIZE];
    char line[BUFFER_SIZE];
    size_t line_len = 0;

    while (running)
    {
        ssize_t n = recv(sock,
                         buffer,
                         sizeof(buffer),
                         0);

        if (n <= 0)
        {
            running = 0;
            break;
        }

        for (ssize_t i = 0; i < n; i++)
        {
            char ch = buffer[i];

            if (ch == '\n')
            {
                line[line_len] = '\0';

                if (line_len > 0)
                {
                    printf("\nServer: %s\n", line);
                    fflush(stdout);
                }

                line_len = 0;
            }
            else if (ch != '\r')
            {
                if (line_len < sizeof(line) - 1)
                {
                    line[line_len++] = ch;
                }
            }
        }
    }

    return NULL;
}

/* --------------------------------------------------------- */
/* Get filename from path                                    */
/* --------------------------------------------------------- */

const char *get_filename(const char *path)
{
    const char *slash;
    const char *backslash;

    slash = strrchr(path, '/');
    backslash = strrchr(path, '\\');

    if (slash != NULL && backslash != NULL)
    {
        return (slash > backslash) ? slash + 1 : backslash + 1;
    }

    if (slash != NULL)
    {
        return slash + 1;
    }

    if (backslash != NULL)
    {
        return backslash + 1;
    }

    return path;
}

/* --------------------------------------------------------- */
/* Send file to server                                       */
/* --------------------------------------------------------- */

int send_file(const char *target,
              const char *filepath)
{
    struct stat file_info;

    if (stat(filepath, &file_info) != 0)
    {
        perror("stat");
        return -1;
    }

    if (!S_ISREG(file_info.st_mode))
    {
        printf("Error: Not a regular file.\n");
        return -1;
    }

    if (file_info.st_size < 0)
    {
        printf("Error: Invalid file size.\n");
        return -1;
    }

    unsigned long long filesize =
        (unsigned long long)file_info.st_size;

    const char *filename =
        get_filename(filepath);

    if (filename == NULL || filename[0] == '\0')
    {
        printf("Error: Invalid filename.\n");
        return -1;
    }

    /* ----------------------------------------------------- */
    /* Open file                                              */
    /* ----------------------------------------------------- */

    FILE *fp = fopen(filepath, "rb");

    if (fp == NULL)
    {
        perror("fopen");
        return -1;
    }

    /* ----------------------------------------------------- */
    /* Create SENDFILE header                                 */
    /* ----------------------------------------------------- */

    char header[BUFFER_SIZE];

    int header_len = snprintf(
        header,
        sizeof(header),
        "SENDFILE %s %s %llu\n",
        target,
        filename,
        filesize
    );

    if (header_len < 0 ||
        (size_t)header_len >= sizeof(header))
    {
        printf("Error: SENDFILE header too long.\n");
        fclose(fp);
        return -1;
    }

    /* ----------------------------------------------------- */
    /* Send header                                            */
    /* ----------------------------------------------------- */

    if (send_all(sock,
                 header,
                 (size_t)header_len) < 0)
    {
        perror("send");
        fclose(fp);
        return -1;
    }

    /* ----------------------------------------------------- */
    /* Send exact raw file bytes                              */
    /* ----------------------------------------------------- */

    char file_buffer[4096];

    size_t bytes_read;

    while ((bytes_read =
                fread(file_buffer,
                      1,
                      sizeof(file_buffer),
                      fp)) > 0)
    {
        if (send_all(sock,
                     file_buffer,
                     bytes_read) < 0)
        {
            perror("send file");
            fclose(fp);
            return -1;
        }
    }

    if (ferror(fp))
    {
        printf("Error while reading file.\n");
        fclose(fp);
        return -1;
    }

    fclose(fp);

    printf("File sent: %s (%llu bytes)\n",
           filename,
           filesize);

    return 0;
}

/* --------------------------------------------------------- */
/* Main                                                       */
/* --------------------------------------------------------- */

int main(void)
{
    struct sockaddr_in server_addr;
    char username[USERNAME_SIZE];
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    /* ----------------------------------------------------- */
    /* Create socket                                          */
    /* ----------------------------------------------------- */

    sock = socket(AF_INET,
                  SOCK_STREAM,
                  0);

    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    /* ----------------------------------------------------- */
    /* Configure server address                               */
    /* ----------------------------------------------------- */

    memset(&server_addr,
           0,
           sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET,
                  SERVER_IP,
                  &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sock);
        return 1;
    }

    /* ----------------------------------------------------- */
    /* Connect                                                */
    /* ----------------------------------------------------- */

    if (connect(sock,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(sock);
        return 1;
    }

    printf("Connected to NetMessenger Server!\n");

    /* ----------------------------------------------------- */
    /* REGISTER                                               */
    /* ----------------------------------------------------- */

    printf("Enter username: ");

    if (scanf("%49s", username) != 1)
    {
        close(sock);
        return 1;
    }

    snprintf(command,
             sizeof(command),
             "REGISTER %s\n",
             username);

    if (send_all(sock,
                 command,
                 strlen(command)) < 0)
    {
        perror("send");
        close(sock);
        return 1;
    }

    /* ----------------------------------------------------- */
    /* Receive registration response                          */
    /* ----------------------------------------------------- */

    memset(response,
           0,
           sizeof(response));

    ssize_t received = recv(sock,
                            response,
                            sizeof(response) - 1,
                            0);

    if (received <= 0)
    {
        perror("recv");
        close(sock);
        return 1;
    }

    response[received] = '\0';

    printf("Server: %s", response);

    /* Check duplicate username / registration error */

    if (strncmp(response,
                "OK REGISTERED",
                13) != 0)
    {
        close(sock);
        return 1;
    }

    /* Remove leftover newline from scanf */

    int ch;

    while ((ch = getchar()) != '\n' &&
           ch != EOF)
    {
        ;
    }

    /* ----------------------------------------------------- */
    /* Start receiver thread                                  */
    /* ----------------------------------------------------- */

    pthread_t receiver_thread;

    if (pthread_create(&receiver_thread,
                       NULL,
                       receive_messages,
                       NULL) != 0)
    {
        perror("pthread_create");
        close(sock);
        return 1;
    }

    /* ----------------------------------------------------- */
    /* Command loop                                           */
    /* ----------------------------------------------------- */

    while (running)
    {
        printf("Enter command: ");
        fflush(stdout);

        if (fgets(command,
                  sizeof(command),
                  stdin) == NULL)
        {
            break;
        }

        command[strcspn(command, "\r\n")] = '\0';

        if (strlen(command) == 0)
        {
            continue;
        }

        /* ------------------------------------------------- */
        /* SENDFILE                                            */
        /* ------------------------------------------------- */

        if (strncmp(command,
                    "SENDFILE ",
                    9) == 0)
        {
            char target[USERNAME_SIZE];
            char filepath[FILENAME_SIZE];

            memset(target, 0, sizeof(target));
            memset(filepath, 0, sizeof(filepath));

            if (sscanf(command + 9,
                       "%49s %255s",
                       target,
                       filepath) != 2)
            {
                printf("Usage: SENDFILE <target> <filepath>\n");
                continue;
            }

            if (send_file(target,
                          filepath) < 0)
            {
                printf("File transfer failed.\n");
            }

            continue;
        }

        /* ------------------------------------------------- */
        /* Normal commands                                    */
        /* ------------------------------------------------- */

        char send_buffer[BUFFER_SIZE];

        int length = snprintf(send_buffer,
                              sizeof(send_buffer),
                              "%s\n",
                              command);

        if (length < 0 ||
            (size_t)length >= sizeof(send_buffer))
        {
            printf("Command too long.\n");
            continue;
        }

        if (send_all(sock,
                     send_buffer,
                     (size_t)length) < 0)
        {
            perror("send");
            break;
        }

        /* ------------------------------------------------- */
        /* QUIT                                                */
        /* ------------------------------------------------- */

        if (strcmp(command, "QUIT") == 0)
        {
            /*
             * Give receiver thread a moment to receive
             * OK BYE before closing the socket.
             */

            sleep(1);

            running = 0;
            break;
        }
    }

    /* ----------------------------------------------------- */
    /* Cleanup                                                */
    /* ----------------------------------------------------- */

    running = 0;

    shutdown(sock,
             SHUT_RDWR);

    close(sock);

    pthread_join(receiver_thread,
                 NULL);

    printf("\nDisconnected from server.\n");

    return 0;
}
