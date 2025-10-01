# Real-time Chating in C

Build a real-time chatroom using C, socket programming, and multi-threading. This project demonstrates a simple server-client architecture where multiple users can communicate instantly across machines.

## Project Overview

This application lets users join a chatroom and send messages in real-time. The server manages user connections using separate threads for each client, ensuring smooth and concurrent messaging. The project is a practical introduction to network programming in C.

## Key Features

- Real-time text-based chat communication
- Supports multiple clients connecting simultaneously
- Commands to message all users, private message, list online users, and exit
- Runs locally or across different machines
- Simple thread-based architecture for concurrency

## Installation

1. Clone the repository:
    ```
    git clone https://github.com/Vanshgh7/real-time-chating
    cd real-time-chating--c
    ```
2. Compile both server and client:
    ```
    gcc server.c -o server -lpthread
    gcc client.c -o client -lpthread
    ```
## Usage

### Running the Server

-open cocal.com ,use terminal and exicute below commanads

- To start the server on port 80 (default):
    ```
    ./server
    ```
- To specify a different port:
    ```
    ./server 83
    ```

### Connecting the Client

- Client usage:
    ```
    ./client [-h] [-a] [-p] [-u]
    ```
- Options:
    - `-h`  Show help message (optional)
    - `-a`  Server IP address (required; use `localhost` for local machine)
    - `-p`  Server port (required)
    - `-u`  Your username (required)

#### Example

## Chat Commands

| Command      | Parameter         | Description                                 |
| ------------ | ---------------  | ------------------------------------------- |
| `quit`       | —                | Exit the chatroom                           |
| `msg`        | "text"           | Send message to all users                   |
| `msg`        | "text" user      | Send private message to a specific user     |
| `online`     | —                | Show all online users                       |
| `help`       | —                | Display help info                           |

## How It Works

- **Server:** Handles each user in a separate thread. All threads safely share user data using a global linked list.
- **Client:** Runs a chatroom shell after connecting. Two threads manage sending commands and receiving messages simultaneously.

## Contributing & Future Improvements

Feel free to open issues or pull requests. Some ideas for improvement:

1.Message History Logging – Save chat messages on the server so users can see past messages when they join or reconnect.

2.File Sharing – Allow users to send small files (like text or images) to others in the chatroom.

3.User Status & Notifications – Add features like “user is typing…”, online/offline status, or sound notifications for new messages.

