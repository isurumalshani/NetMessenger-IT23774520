# IE3010 NetMessenger Structured Reflection

## 1. What I Learned

Through this project, I gained practical experience in TCP/IP socket programming using C and the BSD sockets API. I learned how a TCP server accepts client connections and how clients communicate with the server using a defined application protocol.

I also gained experience with POSIX threads and concurrent client handling. Testing multiple clients helped me understand why concurrency is important in a multi-client server.

## 2. Technical Challenges

One of the main challenges was implementing and testing communication between multiple clients. Different commands such as private messaging, broadcast messaging and room messaging required the server to correctly identify users and route messages.

File sharing was another important challenge because the server had to receive the file data and store it using the required personalised directory structure.

GitHub authentication and repository management from the Ubuntu virtual machine was also a practical challenge. I configured a repository-specific authentication method and successfully pushed the project to GitHub.

## 3. How I Solved the Challenges

I solved the networking issues by testing the server and client repeatedly using separate terminal sessions. Invalid commands and invalid usernames were also tested to verify error handling.

For file sharing, I verified both the client-side transfer and the server-side stored file.

For project management, I used Git commits to track meaningful project documentation and implementation changes.

## 4. Testing Experience

The project was tested using multiple client connections. Registration, LIST, BCAST, PMSG, room operations, file transfer, error handling and QUIT were tested.

A five-client simultaneous connection test was also performed.

The personalised values were verified during testing:

- Registration Number: IT23774520
- Port: 10520
- NID: 7745

## 5. Use of AI Assistance

AI tools were used as supporting resources for understanding networking concepts, debugging, documentation and development guidance.

AI-generated suggestions were reviewed and adapted rather than being accepted without testing. The implementation was compiled and executed in the Ubuntu environment and the functionality was verified through practical testing.

A detailed record of AI assistance is provided in:

`docs/AI_PROMPT_LOG.md`

## 6. Improvements I Would Make

If more development time were available, I would improve the system with stronger authentication, additional security mechanisms, more extensive automated testing and improved file-transfer validation.

I would also improve the user interface of the client application and provide more detailed server-side monitoring.

## 7. Overall Reflection

This project helped me connect theoretical networking concepts with practical software development. I gained experience with TCP sockets, concurrent programming, application-layer protocols, file transfer, error handling, Linux development and GitHub-based version control.

The most valuable part of the project was testing the system with multiple clients because it demonstrated how the different components work together as a complete network application.
