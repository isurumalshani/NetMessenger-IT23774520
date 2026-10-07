# IE3010 NetMessenger Design Diary

## Student Details

- Registration Number: IT23774520
- Project: NetMessenger
- Port: 10520
- NID: 7745

## Project Overview

The NetMessenger project is a TCP/IP multi-client chat and file-sharing platform implemented in C using the standard BSD sockets API. The system consists of a server and multiple clients communicating through TCP.

## Key Design Decisions

### 1. Client-Server Architecture

I selected a client-server architecture where one server manages multiple connected clients. The server is responsible for user registration, messaging, chat rooms, file transfers, connection management and logging.

### 2. Concurrency Model

POSIX threads were selected to handle multiple clients concurrently. A separate thread is used for each connected client. This allows the server to continue serving other clients while one client is communicating or transferring a file.

The server was tested with multiple simultaneous clients, including a five-client connection test.

### 3. Communication Protocol

The given line-based TCP protocol was followed. The implemented commands include REGISTER, LIST, BCAST, PMSG, JOIN, LEAVE, ROOMS, RMSG, SENDFILE and QUIT.

Responses include the personalised NID value `NID:7745`.

### 4. File Storage

Received files are stored using the personalised storage structure:

`storage/IT23774520/<sender_username>/<filename>`

This allows received files to be organised according to the sender.

### 5. Server Logging

A personalised log file is used:

`netmsg_IT23774520.log`

The server records important events such as client connections, registrations, messaging, file transfers and disconnections with timestamps.

## Development and Testing Experience

During development, the server and client programs were compiled and tested on Ubuntu Linux using GCC and POSIX threads. Several protocol operations were tested using multiple client sessions.

Testing included successful registration, duplicate username handling, user listing, broadcast messaging, private messaging, room operations, file transfer, graceful QUIT and error handling.

A file transfer test confirmed that the received file was stored under the personalised storage directory.

## Problems and Solutions

One development challenge was configuring the GitHub repository and authenticating Git operations from the Ubuntu virtual machine. A fine-grained GitHub Personal Access Token was configured with access to the project repository and repository contents read/write permission.

The project was then pushed successfully to the GitHub repository.

Another challenge was ensuring that the personalised project values were consistently used across the server, client, Makefile, log file and storage path.

## Current Progress

The main server and client implementation has been completed and tested. The personalised README and design documentation are being added to the repository.

Further work includes final protocol verification, documentation, implementation report preparation, testing evidence organisation, AI prompt logging and final submission packaging.
