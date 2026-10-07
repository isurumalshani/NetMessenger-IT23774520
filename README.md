# IE3010 NetMessenger - IT23774520

## Project Description

NetMessenger is a TCP/IP multi-client chat and file-sharing platform implemented in C using the BSD sockets API.

The system consists of:
- A multi-client TCP server
- A TCP client application
- Concurrent client handling using POSIX threads
- Broadcast and private messaging
- Chat rooms
- File sharing
- Server-side event logging

## Student Personalisation

| Item | Value |
|---|---|
| Registration Number | IT23774520 |
| Last Four Digits | 4520 |
| Listening Port | 10520 |
| NID | 7745 |
| Server Source | server_4520.c |
| Client Source | client_4520.c |
| Makefile | Makefile_4520 |
| Log File | netmsg_IT23774520.log |
| Storage Path | ./storage/IT23774520/<sender_username>/<filename> |

## Requirements

The project is implemented in C using the standard BSD socket API on Linux.

Required tools:
- GCC
- Make
- POSIX Threads
- Linux TCP/IP networking

## Build

Clone the repository and enter the project directory:

```bash
cd ~/NetMessenger
