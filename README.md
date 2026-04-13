# Multithreaded Chat Application

A fully functional multithreaded chat server and client application in C, developed as part of the Software Systems Assignment 2.

---

## Table of Contents

1. [Overview](#overview)
2. [Compilation](#compilation)
3. [Features Implemented](#features-implemented)
4. [Proposed Extensions](#proposed-extensions)
5. [Extra Features](#extra-features)
6. [Architecture](#architecture)
7. [Usage Examples](#usage-examples)

---

## Overview

This project implements a **multithreaded, multi-client chat application** using UDP sockets for communication. The application demonstrates key concepts in:

- **Concurrency**: Multiple threads handle different aspects of client and server operations
- **Synchronization**: Reader-writer locks and mutex/condition variables protect shared resources
- **Network Communication**: UDP socket programming for client-server interaction

The chat server supports multiple simultaneous clients, broadcast and private messaging, user management features, and automatic inactive client detection.

---

## Compilation

To compile the chat application, navigate to the project directory and run:

```bash
gcc server_interface.c -o server 
```

```bash
gcc client_interface.c -o client
```

To run the server:

In a different/split terminal:
```bash
./server
```
or
```bash
./server &
```
To run a client (in a separate terminal):

```bash
./client
```

---

## Features Implemented

### 1. Client-Server Communication **Fully Implemented**

UDP-based communication between a single server and multiple clients.

**Implementation Details:**
- Server binds to UDP port `12000` (known to all clients)
- Clients bind to any available UDP port (assigned by OS)
- Two-way communication using `sendto()` and `recvfrom()` system calls
- IP address and port number used to identify each client

---

### 2. Client-Side Requirements **Fully Implemented**

Each client spawns multiple threads for concurrent operations.

| Thread | Description |
|--------|-------------|
| Listener Thread | Waits for incoming messages from server, adds to task queue |
| Sender Thread (Pre-connection) | Handles user input before connected (only `conn$` valid) |
| Sender Thread (Post-connection) | Handles user input after connected (all commands valid) |
| Chat Display Thread | Renders the chat UI with incoming messages |
| Queue Manager Thread | Processes incoming server responses |

**Supported Client Commands:**

| Command | Description | Example |
|---------|-------------|---------|
| `conn$ name` | Connect to the chat with given name | `conn$ Alice` |
| `say$ msg` | Send message to all connected clients | `say$ Hello everyone!` |
| `sayto$ name msg` | Send private message to specific client | `sayto$ Bob How are you?` |
| `mute$ name` | Mute messages from specified client | `mute$ Bob` |
| `unmute$ name` | Unmute the specified client | `unmute$ Bob` |
| `rename$ name` | Change the client's display name | `rename$ Alice123` |
| `disconn$` | Disconnect from the server | `disconn$` |
| `kick$ name` | Remove a client (admin only - first connected user) | `kick$ Bob` |
| `:q` | Exit the client application | `:q` |

---

### 3. Server-Side Requirements **Fully Implemented**

The server manages all connected clients and routes messages appropriately.

**Server Architecture:**

| Thread | Description |
|--------|-------------|
| Listener Thread | Continuously receives UDP packets from clients |
| Queue Manager Thread | Dispatches incoming requests to worker threads |
| Worker Threads | Handle individual commands (connect, say, disconnect, etc.) |
| Connection Manager Thread | Monitors client activity and removes inactive clients |

**Server Actions:**

| Request | Server Action | Response |
|---------|---------------|----------|
| `conn$` | Add client to linked list | `connsuccess$ name` |
| `say$` | Broadcast message to all clients | `say$ sender: message` |
| `sayto$` | Send to specific recipient only | `say$ sender:(private message) msg` |
| `disconn$` | Remove client from list | `disconnresponse$` |
| `mute$` | Relay mute command back to client | `mute$ name` |
| `unmute$` | Relay unmute command back to client | `unmute$ name` |
| `rename$` | Update client name in linked list | `rename$ new_name` |
| `kick$` | Remove client (admin only) and notify all | `disconnresponse$` (to kicked) + broadcast |

**Admin System:**
- The first client to connect becomes the admin (head of the linked list)
- Only the admin can use the `kick$` command
- Non-admin users receive a "Permission denied" error if they attempt to kick

---

### 4. Synchronization Requirements **Fully Implemented**

The server's shared resources are protected using synchronization primitives.

**Reader-Writer Lock (Monitor Pattern):**

```c
typedef struct {
    int AR;  // active readers
    int WR;  // waiting readers
    int AW;  // active writers
    int WW;  // waiting writers
    pthread_mutex_t lock;
    pthread_cond_t cond_read;
    pthread_cond_t cond_write;
} Monitor_t;
```

| Operation Type | Examples |
|----------------|----------|
| **Reader Operations** | Broadcasting messages, looking up clients by name/address |
| **Writer Operations** | Adding/removing clients, renaming clients, kicking users |

**Additional Synchronization:**
- `pthread_mutex_t` for client connection state
- `pthread_cond_t` for signaling state changes
- Thread-safe bounded queue with `nonempty`/`nonfull` condition variables
- Maximum worker thread limiting (128 concurrent threads)

---

### 5. User Interface **Fully Implemented**

The client uses terminal escape sequences to create a dedicated chat interface.

**Implementation Details:**
- Alternate screen buffer (`\033[?1049h`) for dedicated chat view
- Clear screen and cursor positioning with ANSI escape codes
- Separate areas for chat messages and user input
- Dynamic prompt showing connection state: `[username] >` or `[Not connected] >`
- Muted messages filtered from display

---

## Proposed Extensions

### PE 1: History at Connection **Fully Implemented**

When a new client connects, they receive the last 15 broadcast messages.

**Implementation Details:**
- Server maintains a shared chat history array (`char** chat_history`)
- Circular buffer logic preserves the most recent 15 messages
- Upon connection, server sends all stored history to the new client
- Messages prefixed with `say$` for consistent client-side handling

```c
#define CHAT_HISTORY_SIZE 15

// Server sends history on connection:
for (int i = start; i < *chat_historyc; i++) {
    snprintf(history_msg, RESPONSE_BUFFER_SIZE, "say$ %s", chat_history[i]);
    udp_socket_write(sd, client_addr, history_msg, RESPONSE_BUFFER_SIZE);
}
```

---

### PE 2: Remove Inactive Clients **Fully Implemented**

The server automatically detects and removes inactive clients.

**Implementation Details:**

| Parameter | Value |
|-----------|-------|
| Inactivity Threshold | 3 minutes (180 seconds) |
| Ping Timeout | 10 seconds |

**Activity Monitoring:**
- Each client node stores `last_active` timestamp
- Every command (except `conn$`) updates the client's activity time
- Connection Manager thread periodically scans for inactive clients

**Ping Mechanism:**
1. Client exceeds inactivity threshold → Server sends `ping$`
2. Client should respond with `retping$`
3. If no response within timeout → Client is disconnected

```c
typedef struct client_node {
    struct client_node* next;
    struct sockaddr_in client_address;
    char client_name[NAME_SIZE];
    time_t last_active;      // Last activity timestamp
    time_t ping_sent;        // Time when ping was sent (0 if not pending)
} client_node_t;
```

---

## Extra Features

These features were **not required** by the assignment but were implemented as enhancements.

### 1. Custom Hash Table for Mute List **Fully Implemented**

Efficient O(1) average-case lookup for muted users.

**Implementation Details:**
- Uses djb2 hash function for string hashing
- Separate chaining for collision resolution
- Client-side mute filtering (server doesn't need to track per-client mute lists)

| Operation | Complexity |
|-----------|------------|
| `insert()` | O(1) average |
| `contains()` | O(1) average |
| `remove_key()` | O(1) average |

---

### 2. Thread-Safe Bounded Queue **Fully Implemented**

A producer-consumer queue for managing incoming requests.

**Features:**
- Fixed capacity (`QUEUE_MAX = 256`)
- Blocking operations with condition variables
- Automatic command tokenization on pop
- Supports optional sender address storage (for server-side use)

```c
typedef struct {
    queue_node_t data[QUEUE_MAX];
    int head, tail, size;
    pthread_mutex_t lock;
    pthread_cond_t nonempty;
    pthread_cond_t nonfull;
} Queue;
```

---

### 3. Command Parser with Type Safety **Fully Implemented**

Structured command handling with enum-based dispatch.

**Supported Command Types:**
```c
typedef enum {
    CONN, CONN_SUCCESS, CONN_FAILED,
    SAY, SAYTO,
    MUTE, UNMUTE,
    RENAME,
    DISCONN, DISCONN_RESPONSE,
    KICK,
    PING, RETPING,
    ERROR, UNKNOWN
} command_kind_t;
```

---

### 4. Retroactive Mute Filtering **Fully Implemented**

When a user is muted, their messages are hidden from **all** chat history — not just future messages.

**Implementation Details:**
- Chat display thread re-renders the entire message list on each update
- Before displaying each message, the sender's username is extracted and checked against the mute table
- If the sender is muted, the message is skipped during rendering
- Unmuting a user immediately restores visibility of their previous messages

```c
// From chat_display() - messages from muted users are filtered out
for (int i = 0; i < total_messages; i++) {
    char username[NAME_SIZE];
    extract_username(chat_args->messages[i], username, NAME_SIZE);
    if (contains(mute_table, username)) continue;  // Skip muted users
    printf("%s\n", chat_args->messages[i]);
}
```

**Benefits:**
- Cleaner chat experience when muting disruptive users
- Reversible - unmute restores all previous messages
- No server-side storage required for mute state

---

### 5. First-Connected Admin System **Fully Implemented**

The first client to connect to the server is automatically granted admin privileges.

**Implementation Details:**
- Admin status determined by position in linked list (head = admin)
- Server checks if requester's address matches the head node before allowing kick
- Non-admins receive an error response when attempting to kick

```c
// Admin check in kick function
client_node_t* head_node = *(cmd_args->head);
bool is_admin = (head_node != NULL && 
                 memcmp(&(head_node->client_address), cmd_args->from_addr, 
                        sizeof(struct sockaddr_in)) == 0);

if (!is_admin) {
    // Send permission denied error
    snprintf(error_response, MAX_MESSAGE, 
             "error$ Permission denied. Only admin can kick users.");
    udp_socket_write(cmd_args->sd, cmd_args->from_addr, error_response, MAX_MESSAGE);
    return NULL;
}
```

**Note:** The assignment suggested using port 6666 for admin, but this implementation uses a first-come-first-served approach which is more practical for real-world usage.

---

### 6. Graceful Connection State Management **Fully Implemented**

Thread-safe handling of connection/disconnection states.

**Features:**
- Mutex-protected `connected` boolean
- Condition variable signaling for state transitions
- Separate input threads for pre-connection and post-connection states
- Automatic prompt updates reflecting connection state

---

## Architecture

### High-Level Architecture
![](./Server_architecture.png)

### Data Flow

![](./Client_server_processing.png)
---

## File Structure

| File | Description |
|------|-------------|
| `server_interface.c` | Main server implementation (listener, queue manager, command handlers) |
| `client_interface.c` | Main client implementation (UI, sender, listener threads) |
| `server_types.h` | Server data structures (Monitor, client linked list, thread args) |
| `client_types.h` | Client data structures and command execution functions |
| `cmd.h` | Command type definitions and parser |
| `queue.h` | Thread-safe bounded queue implementation |
| `udp.h` | UDP socket wrapper functions |
| `custom_hash_table.h` | Hash table for mute list |

---

## Usage Examples

### Basic Chat Session

**Terminal 1 (Server):**
```bash
$ ./server
Socket bound successfully to port 12000
Server is listening on port 12000
```

**Terminal 2 (Client 1 - Alice):**
```bash
$ ./client
[Not connected] > conn$ Alice
Successfully connected as: Alice
[Alice] > say$ Hello everyone!
[Alice] > 
```

**Terminal 3 (Client 2 - Bob):**
```bash
$ ./client
[Not connected] > conn$ Bob
Successfully connected as: Bob
[Bob] > 
Alice: Hello everyone!
[Bob] > say$ Hi Alice!
```

### Private Messaging

```bash
[Alice] > sayto$ Bob How are you doing?
you to Bob:(private message) How are you doing?
```

Bob sees:
```bash
Alice:(private message) How are you doing?
```

### Muting Users

```bash
[Alice] > mute$ Bob
[Alice] >
# Bob's messages will no longer appear in Alice's chat
```

### Renaming

```bash
[Alice] > rename$ Alice123
Successfully renamed as: Alice123
[Alice123] >
```

### Admin Kick (first connected user is admin)

The first user to connect to the server becomes the admin and has kick privileges.

**Admin (first to connect):**
```bash
[Alice] > kick$ Bob
# Bob receives disconnect, all clients see: "SERVER: Bob has been kicked"
```

**Non-admin attempting to kick:**
```bash
[Bob] > kick$ Charlie
Error from server: Permission denied. Only admin can kick users.
```

---

## Summary Table

| Feature | Status |
|---------|--------|
| **Core Features** | |
| UDP Client-Server Communication | Fully Implemented |
| Multi-threaded Client (Sender + Listener) | Fully Implemented |
| Multi-threaded Server (Thread Pool) | Fully Implemented |
| Connect/Disconnect | Fully Implemented |
| Broadcast Messages (`say$`) | Fully Implemented |
| Private Messages (`sayto$`) | Fully Implemented |
| Mute/Unmute Users | Fully Implemented |
| Rename Client | Fully Implemented |
| Admin Kick | Fully Implemented |
| **Synchronization** | |
| Reader-Writer Lock (Linked List) | Fully Implemented |
| Thread-Safe Message Queue | Fully Implemented |
| Maximum Worker Thread Limiting | Fully Implemented |
| **User Interface** | |
| Dedicated Chat Display | Fully Implemented |
| Connection State Prompt | Fully Implemented |
| **Proposed Extensions** | |
| PE 1: History at Connection | Fully Implemented |
| PE 2: Remove Inactive Clients | Fully Implemented |
| **Extra Features** | |
| Custom Hash Table (Mute List) | Fully Implemented |
| Retroactive Mute Filtering | Fully Implemented |
| First-Connected Admin System | Fully Implemented |
| Graceful State Transitions | Fully Implemented |
| Command Type Parser | Fully Implemented |

---

## Authors

Joshua Hirschkorn (CID: 02378306)| Wells [ADD SURNAME]

---

## References

- `man` pages for system calls: `socket`, `bind`, `sendto`, `recvfrom`, `pthread_create`, `pthread_mutex_lock`, `pthread_cond_wait`
- Lecture materials (Lectures 5–8 for concurrency, Lectures 15–16 for communication)
- Assignment 2 specification document
