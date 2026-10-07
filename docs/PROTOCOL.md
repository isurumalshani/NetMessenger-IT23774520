# IE3010 NetMessenger Protocol Implementation

## Personalisation

- Registration Number: IT23774520
- Port: 10520
- NID: 7745

## Protocol

NetMessenger uses the required line-based TCP protocol. Text commands and responses are terminated with a newline. SENDFILE is followed by the required raw file bytes.

## REGISTER

```text
REGISTER <username>
