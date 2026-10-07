# IE3010 NetMessenger Implementation Report

## 1. Student and Personalisation Details

- Registration Number: IT23774520
- Last Four Digits: 4520
- Listening Port: 10520
- NID: 7745
- Server: server_4520.c
- Client: client_4520.c
- Makefile: Makefile_4520
- Log File: netmsg_IT23774520.log
- Storage Path: storage/IT23774520/<sender_username>/<filename>

## 2. Project Overview

NetMessenger is a TCP/IP multi-client chat and file-sharing platform implemented in C using the standard BSD sockets API.

The system consists of one TCP server and multiple clients.

## 3. Architecture

The system follows a client-server architecture.

The server:
- Accepts TCP client connections.
- Maintains registered users.
- Processes protocol commands.
- Handles chat rooms.
- Routes messages.
- Handles file transfers.
- Stores received files.
- Records server events in the personalised log file.

Clients connect to the server using TCP.

### Concurrency Model

POSIX threads are used to handle multiple connected clients concurrently. A separate client-handling thread allows the server to communicate with multiple clients without blocking the entire server.

A five-client simultaneous connection test was performed.

## 4. Personalised Configuration

| Item | Value |
|---|---|
| Registration | IT23774520 |
| Port | 10520 |
| NID | 7745 |
| Server | server_4520.c |
| Client | client_4520.c |
| Makefile | Makefile_4520 |
| Log | netmsg_IT23774520.log |
| Storage | storage/IT23774520/ |

## 5. Protocol Implementation

The required commands implemented and tested are:

- REGISTER
- LIST
- BCAST
- PMSG
- JOIN
- LEAVE
- ROOMS
- RMSG
- SENDFILE
- QUIT

The implementation uses the personalised NID value `NID:7745` in server responses.

## 6. Functional Testing

The following functions were tested:

- User registration
- Duplicate username handling
- User listing
- Broadcast messaging
- Private messaging
- Invalid user error handling
- Room creation and joining
- Room listing
- Room messaging
- Leaving rooms
- File transfer
- File storage
- Graceful QUIT
- Server logging
- Five simultaneous clients

Detailed results are documented in:

`docs/TESTING_SUMMARY.md`

## 7. File Sharing

The SENDFILE functionality transfers a file through the server.

The server stores the received file under:

`storage/IT23774520/<sender_username>/<filename>`

The transferred file was checked after the transfer.

## 8. Error Handling

The implementation handles invalid or incomplete commands and invalid users/rooms with error responses rather than terminating the server.

Example verified error:

`ERR 400 INVALID_COMMAND NID:7745`

Another tested error:

`ERR 002 USER_NOT_FOUND NID:7745`

## 9. Server Logging

The server writes timestamped events to:

`netmsg_IT23774520.log`

The log records important events including connections, registrations, file transfers and disconnections.

## 10. Build and Execution

Build using:

```bash
make -f Makefile_4520
