# IE3010 NetMessenger Testing Summary

## Student Details

- Registration Number: IT23774520
- Port: 10520
- NID: 7745

## Test Cases

| Test | Action | Expected Result | Actual Result | Status |
|---|---|---|---|---|
| T01 | Start server on port 10520 | Server starts and listens on port 10520 | Server successfully listened on port 10520 | PASS |
| T02 | Register a new username | Server accepts unique username | User successfully registered | PASS |
| T03 | Register duplicate username | Server returns USERNAME_TAKEN error | `ERR 001 USERNAME_TAKEN NID:7745` returned | PASS |
| T04 | Execute LIST | Server returns connected users | User list returned successfully | PASS |
| T05 | Send BCAST message | Other connected clients receive broadcast | Broadcast message delivered successfully | PASS |
| T06 | Send PMSG to valid user | Target user receives private message | Private message delivered successfully | PASS |
| T07 | Send PMSG to invalid user | Server returns USER_NOT_FOUND error | `ERR 002 USER_NOT_FOUND NID:7745` returned | PASS |
| T08 | JOIN a room | Client joins or creates the room | Room joined successfully | PASS |
| T09 | Execute ROOMS | Server lists available rooms | Room list returned successfully | PASS |
| T10 | Send RMSG | Room members receive the message | Room message delivered successfully | PASS |
| T11 | LEAVE a room | Client leaves the room | Client left successfully | PASS |
| T12 | Send a file using SENDFILE | Target receives complete file and server stores a copy | File received and stored successfully | PASS |
| T13 | Execute QUIT | Server responds and closes connection cleanly | `OK BYE NID:7745` returned | PASS |
| T14 | Check server log | Server records events with timestamps | `netmsg_IT23774520.log` contained timestamped events | PASS |
| T15 | Connect five clients simultaneously | Server handles multiple clients without crashing | Five simultaneous client connections were tested successfully | PASS |

## File Transfer Verification

A test file was transferred using the SENDFILE functionality.

The server stored the received file under the personalised storage structure:

```text
storage/IT23774520/<sender_username>/<filename>
