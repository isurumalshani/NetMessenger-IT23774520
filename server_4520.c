#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define PORT 10520
#define BACKLOG 10
#define MAX_CLIENTS 100
#define MAX_ROOMS 50
#define BUFFER_SIZE 1024
#define USERNAME_SIZE 50
#define ROOM_SIZE 50
#define FILENAME_SIZE 200
#define MAX_FILE_SIZE (10 * 1024 * 1024)

#define REGISTRATION "IT23774520"
#define NID "7745"

typedef struct
{
    int fd;
    char username[USERNAME_SIZE];
    int registered;
    char room[ROOM_SIZE];
} Client;

typedef struct
{
    char name[ROOM_SIZE];
    int active;
} Room;

Client clients[MAX_CLIENTS];
Room rooms[MAX_ROOMS];

int client_count = 0;
int room_count = 0;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* --------------------------------------------------------- */
/* Timestamped server log                                    */
/* --------------------------------------------------------- */

void write_log(const char *message)
{
    FILE *fp;
    time_t now;
    struct tm *tm_info;
    char timestamp[64];

    time(&now);
    tm_info = localtime(&now);

    strftime(timestamp,
             sizeof(timestamp),
             "%Y-%m-%d %H:%M:%S",
             tm_info);

    pthread_mutex_lock(&log_mutex);

    fp = fopen("netmsg_IT23774520.log", "a");

    if (fp != NULL)
    {
        fprintf(fp, "[%s] %s\n", timestamp, message);
        fclose(fp);
    }

    pthread_mutex_unlock(&log_mutex);
}

/* --------------------------------------------------------- */
/* Send standard server response with NID                    */
/* --------------------------------------------------------- */

void send_response(int fd, const char *message)
{
    char response[BUFFER_SIZE];

    snprintf(response,
             sizeof(response),
             "%s NID:%s\n",
             message,
             NID);

    send(fd, response, strlen(response), 0);
}

/* --------------------------------------------------------- */
/* Send forwarded MSG without NID                            */
/* --------------------------------------------------------- */

void send_message(int fd, const char *message)
{
    send(fd, message, strlen(message), 0);
}

/* --------------------------------------------------------- */
/* Read one newline-terminated line                          */
/* --------------------------------------------------------- */

int recv_line(int fd, char *buffer, size_t size)
{
    size_t pos = 0;
    char ch;
    ssize_t n;

    while (pos < size - 1)
    {
        n = recv(fd, &ch, 1, 0);

        if (n <= 0)
        {
            return -1;
        }

        if (ch == '\n')
        {
            break;
        }

        if (ch != '\r')
        {
            buffer[pos++] = ch;
        }
    }

    buffer[pos] = '\0';

    return (int)pos;
}

/* --------------------------------------------------------- */
/* Receive exact number of bytes                            */
/* --------------------------------------------------------- */

int recv_exact(int fd, void *buffer, size_t size)
{
    size_t total = 0;
    char *ptr = (char *)buffer;

    while (total < size)
    {
        ssize_t n = recv(fd,
                         ptr + total,
                         size - total,
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
/* Find client by username                                   */
/* --------------------------------------------------------- */

int find_client_by_username(const char *username)
{
    int i;

    for (i = 0; i < client_count; i++)
    {
        if (clients[i].registered &&
            strcmp(clients[i].username, username) == 0)
        {
            return i;
        }
    }

    return -1;
}

/* --------------------------------------------------------- */
/* Find room by name                                         */
/* --------------------------------------------------------- */

int find_room(const char *room_name)
{
    int i;

    for (i = 0; i < room_count; i++)
    {
        if (rooms[i].active &&
            strcmp(rooms[i].name, room_name) == 0)
        {
            return i;
        }
    }

    return -1;
}

/* --------------------------------------------------------- */
/* Check safe filename                                       */
/* --------------------------------------------------------- */

int valid_filename(const char *filename)
{
    if (filename == NULL || filename[0] == '\0')
    {
        return 0;
    }

    if (strstr(filename, "..") != NULL)
    {
        return 0;
    }

    if (strchr(filename, '/') != NULL)
    {
        return 0;
    }

    if (strchr(filename, '\\') != NULL)
    {
        return 0;
    }

    return 1;
}

/* --------------------------------------------------------- */
/* Broadcast presence message                                */
/* --------------------------------------------------------- */

void broadcast_presence(const char *username,
                         const char *status,
                         int exclude_fd)
{
    char message[BUFFER_SIZE];
    int i;

    snprintf(message,
             sizeof(message),
             "MSG PRESENCE %s %s\n",
             username,
             status);

    pthread_mutex_lock(&clients_mutex);

    for (i = 0; i < client_count; i++)
    {
        if (clients[i].registered &&
            clients[i].fd != exclude_fd)
        {
            send_message(clients[i].fd, message);
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

/* --------------------------------------------------------- */
/* Client handler                                             */
/* --------------------------------------------------------- */

void *handle_client(void *arg)
{
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    int index = -1;
    int registered_here = 0;

    /* Add client to global client table */

    pthread_mutex_lock(&clients_mutex);

    if (client_count >= MAX_CLIENTS)
    {
        pthread_mutex_unlock(&clients_mutex);

        send_response(client_fd,
                      "ERR 500 SERVER_FULL");

        close(client_fd);
        return NULL;
    }

    index = client_count;

    clients[index].fd = client_fd;
    clients[index].registered = 0;
    clients[index].username[0] = '\0';
    clients[index].room[0] = '\0';

    client_count++;

    pthread_mutex_unlock(&clients_mutex);

    write_log("New client connected");

    /* ----------------------------------------------------- */
    /* REGISTER must be first command                        */
    /* ----------------------------------------------------- */

    if (recv_line(client_fd,
                  buffer,
                  sizeof(buffer)) <= 0)
    {
        goto disconnect;
    }

    if (strncmp(buffer, "REGISTER ", 9) != 0)
    {
        send_response(client_fd,
                      "ERR 400 INVALID_COMMAND");

        goto disconnect;
    }

    char username[USERNAME_SIZE];

    memset(username, 0, sizeof(username));

    if (sscanf(buffer + 9,
               "%49s",
               username) != 1)
    {
        send_response(client_fd,
                      "ERR 400 INVALID_COMMAND");

        goto disconnect;
    }

    /* Check duplicate username */

    pthread_mutex_lock(&clients_mutex);

    if (find_client_by_username(username) != -1)
    {
        pthread_mutex_unlock(&clients_mutex);

        send_response(client_fd,
                      "ERR 001 USERNAME_TAKEN");

        goto disconnect;
    }

    strncpy(clients[index].username,
            username,
            USERNAME_SIZE - 1);

    clients[index].username[USERNAME_SIZE - 1] = '\0';

    clients[index].registered = 1;

    pthread_mutex_unlock(&clients_mutex);

    registered_here = 1;

    printf("User registered: %s\n",
           username);

    {
        char log_message[BUFFER_SIZE];

        snprintf(log_message,
                 sizeof(log_message),
                 "User registered: %s",
                 username);

        write_log(log_message);
    }

    {
        char response[BUFFER_SIZE];

        snprintf(response,
                 sizeof(response),
                 "OK REGISTERED %s",
                 username);

        send_response(client_fd, response);
    }

    broadcast_presence(username,
                       "JOINED",
                       client_fd);

    /* ----------------------------------------------------- */
    /* Command loop                                          */
    /* ----------------------------------------------------- */

    while (1)
    {
        int n;

        n = recv_line(client_fd,
                      buffer,
                      sizeof(buffer));

        if (n <= 0)
        {
            break;
        }

        if (strlen(buffer) == 0)
        {
            continue;
        }

        /* ------------------------------------------------- */
        /* LIST                                                */
        /* ------------------------------------------------- */

        if (strcmp(buffer, "LIST") == 0)
        {
            char user_list[BUFFER_SIZE];
            int first_user = 1;
            int i;

            strcpy(user_list, "OK USERS ");

            pthread_mutex_lock(&clients_mutex);

            for (i = 0; i < client_count; i++)
            {
                if (clients[i].registered)
                {
                    if (!first_user)
                    {
                        strncat(user_list,
                                ",",
                                sizeof(user_list) -
                                strlen(user_list) - 1);
                    }

                    strncat(user_list,
                            clients[i].username,
                            sizeof(user_list) -
                            strlen(user_list) - 1);

                    first_user = 0;
                }
            }

            pthread_mutex_unlock(&clients_mutex);

            send_response(client_fd,
                          user_list);

            write_log("LIST command executed");
        }

        /* ------------------------------------------------- */
        /* BCAST                                               */
        /* ------------------------------------------------- */

        else if (strncmp(buffer, "BCAST ", 6) == 0)
        {
            char message[BUFFER_SIZE];

            strncpy(message,
                    buffer + 6,
                    sizeof(message) - 1);

            message[sizeof(message) - 1] = '\0';

            {
                char broadcast_message[BUFFER_SIZE];

                snprintf(broadcast_message,
                         sizeof(broadcast_message),
                         "MSG BCAST %s %s\n",
                         clients[index].username,
                         message);

                pthread_mutex_lock(&clients_mutex);

                int i;

                for (i = 0; i < client_count; i++)
                {
                    if (clients[i].registered &&
                        clients[i].fd != client_fd)
                    {
                        send_message(clients[i].fd,
                                     broadcast_message);
                    }
                }

                pthread_mutex_unlock(&clients_mutex);
            }

            send_response(client_fd,
                          "OK SENT");

            write_log("BCAST message sent");
        }

        /* ------------------------------------------------- */
        /* PMSG                                                */
        /* ------------------------------------------------- */

        else if (strncmp(buffer, "PMSG ", 5) == 0)
        {
            char target[USERNAME_SIZE];
            char message[BUFFER_SIZE];

            memset(target, 0, sizeof(target));
            memset(message, 0, sizeof(message));

            char *space = strchr(buffer + 5, ' ');

            if (space == NULL)
            {
                send_response(client_fd,
                              "ERR 400 INVALID_COMMAND");

                continue;
            }

            size_t target_len =
                (size_t)(space - (buffer + 5));

            if (target_len >= sizeof(target))
            {
                send_response(client_fd,
                              "ERR 400 INVALID_COMMAND");

                continue;
            }

            memcpy(target,
                   buffer + 5,
                   target_len);

            target[target_len] = '\0';

            strncpy(message,
                    space + 1,
                    sizeof(message) - 1);

            message[sizeof(message) - 1] = '\0';

            pthread_mutex_lock(&clients_mutex);

            int target_index =
                find_client_by_username(target);

            if (target_index == -1)
            {
                pthread_mutex_unlock(&clients_mutex);

                send_response(client_fd,
                              "ERR 002 USER_NOT_FOUND");

                continue;
            }

            {
                char private_message[BUFFER_SIZE];

                snprintf(private_message,
                         sizeof(private_message),
                         "MSG PRIV %s %s\n",
                         clients[index].username,
                         message);

                send_message(clients[target_index].fd,
                             private_message);
            }

            pthread_mutex_unlock(&clients_mutex);

            send_response(client_fd,
                          "OK SENT");

            write_log("PMSG sent");
        }

        /* ------------------------------------------------- */
        /* JOIN                                                */
        /* ------------------------------------------------- */

        else if (strncmp(buffer, "JOIN ", 5) == 0)
        {
            char room_name[ROOM_SIZE];

            memset(room_name, 0, sizeof(room_name));

            if (sscanf(buffer + 5,
                       "%49s",
                       room_name) != 1)
            {
                send_response(client_fd,
                              "ERR 400 INVALID_COMMAND");

                continue;
            }

            pthread_mutex_lock(&rooms_mutex);

            int room_index = find_room(room_name);

            if (room_index == -1)
            {
                if (room_count >= MAX_ROOMS)
                {
                    pthread_mutex_unlock(&rooms_mutex);

                    send_response(client_fd,
                                  "ERR 500 SERVER_FULL");

                    continue;
                }

                room_index = room_count;

                strncpy(rooms[room_index].name,
                        room_name,
                        ROOM_SIZE - 1);

                rooms[room_index].name[ROOM_SIZE - 1] =
                    '\0';

                rooms[room_index].active = 1;

                room_count++;
            }

            pthread_mutex_unlock(&rooms_mutex);

            pthread_mutex_lock(&clients_mutex);

            strncpy(clients[index].room,
                    room_name,
                    ROOM_SIZE - 1);

            clients[index].room[ROOM_SIZE - 1] = '\0';

            pthread_mutex_unlock(&clients_mutex);

            {
                char response[BUFFER_SIZE];

                snprintf(response,
                         sizeof(response),
                         "OK JOINED %s",
                         room_name);

                send_response(client_fd,
                              response);
            }

            write_log("Client joined room");
        }

        /* ------------------------------------------------- */
        /* LEAVE                                               */
        /* ------------------------------------------------- */

        else if (strncmp(buffer, "LEAVE ", 6) == 0)
        {
            char room_name[ROOM_SIZE];

            memset(room_name, 0, sizeof(room_name));

            if (sscanf(buffer + 6,
                       "%49s",
                       room_name) != 1)
            {
                send_response(client_fd,
                              "ERR 400 INVALID_COMMAND");

                continue;
            }

            pthread_mutex_lock(&rooms_mutex);

            int room_index = find_room(room_name);

            pthread_mutex_unlock(&rooms_mutex);

            if (room_index == -1)
            {
                send_response(client_fd,
                              "ERR 003 ROOM_NOT_FOUND");

                continue;
            }

            pthread_mutex_lock(&clients_mutex);

            if (strcmp(clients[index].room,
                       room_name) == 0)
            {
                clients[index].room[0] = '\0';
            }

            pthread_mutex_unlock(&clients_mutex);

            {
                char response[BUFFER_SIZE];

                snprintf(response,
                         sizeof(response),
                         "OK LEFT %s",
                         room_name);

                send_response(client_fd,
                              response);
            }

            write_log("Client left room");
        }

        /* ------------------------------------------------- */
        /* ROOMS                                               */
        /* ------------------------------------------------- */

        else if (strcmp(buffer, "ROOMS") == 0)
        {
            char room_list[BUFFER_SIZE];
            int first_room = 1;
            int i;

            strcpy(room_list, "OK ROOMS ");

            pthread_mutex_lock(&rooms_mutex);

            for (i = 0; i < room_count; i++)
            {
                if (rooms[i].active)
                {
                    if (!first_room)
                    {
                        strncat(room_list,
                                ",",
                                sizeof(room_list) -
                                strlen(room_list) - 1);
                    }

                    strncat(room_list,
                            rooms[i].name,
                            sizeof(room_list) -
                            strlen(room_list) - 1);

                    first_room = 0;
                }
            }

            pthread_mutex_unlock(&rooms_mutex);

            send_response(client_fd,
                          room_list);

            write_log("ROOMS command executed");
        }

        /* ------------------------------------------------- */
        /* RMSG                                                */
        /* ------------------------------------------------- */

        else if (strncmp(buffer, "RMSG ", 5) == 0)
        {
            char room_name[ROOM_SIZE];
            char message[BUFFER_SIZE];

            memset(room_name, 0, sizeof(room_name));
            memset(message, 0, sizeof(message));

            char *space = strchr(buffer + 5, ' ');

            if (space == NULL)
            {
                send_response(client_fd,
                              "ERR 400 INVALID_COMMAND");

                continue;
            }

            size_t room_len =
                (size_t)(space - (buffer + 5));

            if (room_len >= sizeof(room_name))
            {
                send_response(client_fd,
                              "ERR 400 INVALID_COMMAND");

                continue;
            }

            memcpy(room_name,
                   buffer + 5,
                   room_len);

            room_name[room_len] = '\0';

            strncpy(message,
                    space + 1,
                    sizeof(message) - 1);

            message[sizeof(message) - 1] = '\0';

            pthread_mutex_lock(&rooms_mutex);

            int room_index = find_room(room_name);

            pthread_mutex_unlock(&rooms_mutex);

            if (room_index == -1)
            {
                send_response(client_fd,
                              "ERR 003 ROOM_NOT_FOUND");

                continue;
            }

            pthread_mutex_lock(&clients_mutex);

            if (strcmp(clients[index].room,
                       room_name) != 0)
            {
                pthread_mutex_unlock(&clients_mutex);

                send_response(client_fd,
                              "ERR 003 ROOM_NOT_FOUND");

                continue;
            }

            {
                char room_message[BUFFER_SIZE];

                snprintf(room_message,
                         sizeof(room_message),
                         "MSG ROOM %s %s %s\n",
                         room_name,
                         clients[index].username,
                         message);

                int i;

                for (i = 0; i < client_count; i++)
                {
                    if (clients[i].registered &&
                        strcmp(clients[i].room,
                               room_name) == 0 &&
                        clients[i].fd != client_fd)
                    {
                        send_message(clients[i].fd,
                                     room_message);
                    }
                }
            }

            pthread_mutex_unlock(&clients_mutex);

            send_response(client_fd,
                          "OK SENT");

            write_log("RMSG sent");
        }

        /* ------------------------------------------------- */
        /* SENDFILE                                            */
        /* ------------------------------------------------- */

        else if (strncmp(buffer, "SENDFILE ", 9) == 0)
        {
            char target[USERNAME_SIZE];
            char filename[FILENAME_SIZE];
            unsigned long long filesize;

            memset(target, 0, sizeof(target));
            memset(filename, 0, sizeof(filename));

            if (sscanf(buffer + 9,
                       "%49s %199s %llu",
                       target,
                       filename,
                       &filesize) != 3)
            {
                send_response(client_fd,
                              "ERR 400 INVALID_COMMAND");

                continue;
            }

            if (filesize > MAX_FILE_SIZE)
            {
                send_response(client_fd,
                              "ERR 004 FILE_TOO_LARGE");

                continue;
            }

            if (!valid_filename(filename))
            {
                send_response(client_fd,
                              "ERR 400 INVALID_COMMAND");

                continue;
            }

            /* Check target */

            int target_is_user = 0;
            int target_index = -1;

            pthread_mutex_lock(&clients_mutex);

            target_index = find_client_by_username(target);

            if (target_index != -1)
            {
                target_is_user = 1;
            }

            pthread_mutex_unlock(&clients_mutex);

            pthread_mutex_lock(&rooms_mutex);

            int target_room_index = find_room(target);

            pthread_mutex_unlock(&rooms_mutex);

            if (!target_is_user &&
                target_room_index == -1)
            {
                send_response(client_fd,
                              "ERR 002 USER_NOT_FOUND");

                continue;
            }

            /* Create sender storage directory */

            char base_dir[512];
            char sender_dir[512];
            char filepath[1024];

            snprintf(base_dir,
                     sizeof(base_dir),
                     "storage/%s",
                     REGISTRATION);

            mkdir("storage", 0755);
            mkdir(base_dir, 0755);

            snprintf(sender_dir,
                     sizeof(sender_dir),
                     "%s/%s",
                     base_dir,
                     clients[index].username);

            mkdir(sender_dir, 0755);

            snprintf(filepath,
                     sizeof(filepath),
                     "%s/%s",
                     sender_dir,
                     filename);

            FILE *fp = fopen(filepath, "wb");

            if (fp == NULL)
            {
                send_response(client_fd,
                              "ERR 500 FILE_STORAGE_ERROR");

                continue;
            }

            char file_buffer[4096];
            unsigned long long remaining = filesize;

            int file_error = 0;

            while (remaining > 0)
            {
                size_t chunk;

                if (remaining > sizeof(file_buffer))
                {
                    chunk = sizeof(file_buffer);
                }
                else
                {
                    chunk = (size_t)remaining;
                }

                if (recv_exact(client_fd,
                               file_buffer,
                               chunk) < 0)
                {
                    file_error = 1;
                    break;
                }

                if (fwrite(file_buffer,
                            1,
                            chunk,
                            fp) != chunk)
                {
                    file_error = 1;
                    break;
                }

                remaining -= chunk;
            }

            fclose(fp);

            if (file_error)
            {
                remove(filepath);

                write_log("File transfer failed");

                close(client_fd);
                goto disconnect;
            }

            /* File successfully stored */

            {
                char log_message[BUFFER_SIZE];

                snprintf(log_message,
                         sizeof(log_message),
                         "File received: %s from %s",
                         filename,
                         clients[index].username);

                write_log(log_message);
            }

            

            
            {
                char response[BUFFER_SIZE];

                snprintf(response,
                         sizeof(response),
                         "OK FILE_RECEIVED %s",
                         filename);

                send_response(client_fd,
                              response);
            }

            /* Notify target */

            {
                char file_message[BUFFER_SIZE];

                snprintf(file_message,
                         sizeof(file_message),
                         "MSG FILE %s %s %llu\n",
                         clients[index].username,
                         filename,
                         filesize);

                pthread_mutex_lock(&clients_mutex);

                if (target_is_user)
                {
                    if (clients[target_index].registered)
                    {
                        send_message(clients[target_index].fd,
                                     file_message);
                    }
                }
                else
                {
                    int i;

                    for (i = 0; i < client_count; i++)
                    {
                        if (clients[i].registered &&
                            strcmp(clients[i].room,
                                   target) == 0 &&
                            clients[i].fd != client_fd)
                        {
                            send_message(clients[i].fd,
                                         file_message);
                        }
                    }
                }

                pthread_mutex_unlock(&clients_mutex);
            }
        }

        /* ------------------------------------------------- */
        /* QUIT                                                */
        /* ------------------------------------------------- */

        else if (strcmp(buffer, "QUIT") == 0)
        {
            send_response(client_fd,
                          "OK BYE");

            write_log("Client sent QUIT");

            break;
        }

        /* ------------------------------------------------- */
        /* Invalid command                                    */
        /* ------------------------------------------------- */

        else
        {
            send_response(client_fd,
                          "ERR 400 INVALID_COMMAND");
        }
    }

disconnect:

    if (registered_here)
    {
        char leaving_username[USERNAME_SIZE];

        pthread_mutex_lock(&clients_mutex);

        strncpy(leaving_username,
                clients[index].username,
                sizeof(leaving_username) - 1);

        leaving_username[sizeof(leaving_username) - 1] = '\0';

        clients[index].registered = 0;
        clients[index].username[0] = '\0';
        clients[index].room[0] = '\0';

        pthread_mutex_unlock(&clients_mutex);

        broadcast_presence(leaving_username,
                           "LEFT",
                           client_fd);

        {
            char log_message[BUFFER_SIZE];

            snprintf(log_message,
                     sizeof(log_message),
                     "Client disconnected: %s",
                     leaving_username);

            write_log(log_message);
        }
    }

    pthread_mutex_lock(&clients_mutex);

    clients[index].registered = 0;
    clients[index].username[0] = '\0';
    clients[index].room[0] = '\0';

    pthread_mutex_unlock(&clients_mutex);

    close(client_fd);

    printf("Client disconnected.\n");

    return NULL;
}

/* --------------------------------------------------------- */
/* Main server                                               */
/* --------------------------------------------------------- */

int main(void)
{
    int server_fd;
    int opt = 1;

    struct sockaddr_in server_addr;

    memset(clients, 0, sizeof(clients));
    memset(rooms, 0, sizeof(rooms));

    server_fd = socket(AF_INET,
                       SOCK_STREAM,
                       0);

    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    if (setsockopt(server_fd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &opt,
                   sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    memset(&server_addr,
           0,
           sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr =
        htonl(INADDR_ANY);

    server_addr.sin_port =
        htons(PORT);

    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd,
               BACKLOG) < 0)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("====================================\n");
    printf("NetMessenger TCP Server Started\n");
    printf("Registration: %s\n", REGISTRATION);
    printf("NID: %s\n", NID);
    printf("Port: %d\n", PORT);
    printf("====================================\n");

    write_log("NetMessenger server started");

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len =
            sizeof(client_addr);

        int *client_fd =
            malloc(sizeof(int));

        if (client_fd == NULL)
        {
            perror("malloc");
            continue;
        }

        *client_fd =
            accept(server_fd,
                   (struct sockaddr *)&client_addr,
                   &client_len);

        if (*client_fd < 0)
        {
            perror("accept");
            free(client_fd);
            continue;
        }

        printf("New client connected: %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        pthread_t thread;

        if (pthread_create(&thread,
                           NULL,
                           handle_client,
                           client_fd) != 0)
        {
            perror("pthread_create");
            close(*client_fd);
            free(client_fd);
            continue;
        }

        pthread_detach(thread);
    }

    close(server_fd);

    return 0;
}
