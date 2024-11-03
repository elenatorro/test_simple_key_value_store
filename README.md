# Solving a 2-Hour Technical Test with ChatGPT

## What's This All About?

[Backend Core Test](https://gist.github.com/javisantana/d94765631591d788b6f2b816913ea08f)

This code test was designed for a Core position at Tinybird. At the time of writing, I'm already working there (mostly on backend stuff) and I'm not evaluating candidates for this role. I just wanted to see how someone might approach this specific test. Spoiler alert: it was a fun challenge, and yes, it took me way longer than 2 hours (closer to 4).

Given my limited C/C++ skills, I decided to use ChatGPT for guidance. This is within the allowed rules, as long as you document how it's used. So, here’s a breakdown of my journey.

## Goals

Time constraint: 2 hours (which, as you know, stretched to more). With that in mind, I defined these main goals:

- Create a client that can use SET and GET commands to store and retrieve values by key.
- Ensure the server stores data in memory for fast retrieval using a simple key-value structure.
- Implement file-based persistence to periodically save data.
- Synchronize data across multiple servers.
- Enable any server to handle client requests and return up-to-date data.

## Project Limitations

To keep things simple and focused, I established these initial constraints:

- The server only supports string values.
- No type conversion is applied.
- No length checks for input data.
- No compression techniques are used.
- No caching on the client or server.
- Client requests are handled using round-robin load balancing.

These limitations were set to keep the basics functional. Further enhancements could be added later.

## Step-by-Step Implementation

## Step 1: Basic Server and Client

First, I needed a basic setup:

> Prompt: “Create a basic HTTP server that listens on a specific port in C++.”

ChatGPT delivered the initial code, compile instructions, and run commands.

> Prompt: “Create a client that can send commands to this server.”

After some tweaks (e.g., preventing the connection from closing after the first command), I had a simple, functioning client-server setup.

## Step 2: Storing Data in Memory

With the client and server communicating, the next step was to store data in memory for quick access.

> Prompt: “Add in-memory data storage using a key-value structure.”

This provided the foundation for SET and GET commands.

## Step 3: Data Persistence

Now, onto saving data between server restarts:

> Prompt: “Modify the server to persist data in a file upon shutdown.”

ChatGPT’s solution worked, but I needed continuous saving.

> Prompt: “Persist data to a file every second while the server is running.”

## Step 4: Handling Multiple Clients

Scaling up meant supporting multiple clients.

> Prompt: “Allow multiple clients to write to the server concurrently.”

ChatGPT introduced threading to the server to manage concurrent client connections and provided guidance on using a mutex lock to ensure safe access to the in-memory data.

## Step 5: Supporting Multiple Servers

This step was where things got more complex:

Challenges: Synchronization, load balancing, high availability, and data integrity.

An alternative approach could have involved using a central database, but I decided to continue with direct file-based persistence.

Note: At this point, I realized that the test might have intended for me to build the database system itself rather than my current approach. In hindsight, I should have clarified this. However, since I didn't, I decided to continue and complete what I had started.

> Prompt: “I want to handle N servers. Let's start with 3. I want the servers to synchronize what they have stored in memory every second among them. I want the servers to keep saving their content from memory into the files every second”

The setup worked locally with hardcoded IPs ({"127.0.0.2", "127.0.0.3", "127.0.0.4"}). Ideally, these should be configured via a file, but I left that for future improvement.

ChatGPT initially used INADDR_ANY, which didn’t work as expected. I researched and requested:

> Prompt: “Bind each server to a specific IP instead of using INADDR_ANY.”

"With multiple servers in place, I needed to update the client:

> Prompt: “I want the client to send requests to different servers using round-robin.”

While testing this, I noticed my synchronization method was flawed; servers were overwriting existing data instead of updating. I did a small refactor so that updates were pushed to other servers immediately upon data receipt.

## Step 6: Final Refactoring

At this point I didn't need ChatGPT help since it was just refactoring.

- I ended up adding support for a DELETE operation on this step, which was not an initial goal.
- On startup, a server checks for existing peers to sync data. If no peers are found, it loads data from its local file. New servers can join seamlessly.
- The client was not handling server connection errors properly. I refactored it so that if a client’s request fails, the system tries another server.

To synchronize the servers, I refactored the function that handles client commands so it could also manage server-to-server communication. This approach allowed the servers to use the same function for both client and internal synchronization commands. To maintain simplicity and keep everything on a single port, I reused the socket already handling client requests, enabling both client and server commands to share the same connection.

The server processes each client request on a separate thread and uses a mutex lock to ensure data integrity when accessing the data store. However, there is no thread or rate limiting to prevent potential saturation by clients. This was intentional, so I did not make any further changes in this area.

## Conclusions

Keeping this within 2 hours was a challenge. Once you get the basics down, the temptation to keep improving is strong! While it's far from perfect, the process helped me explore and learn a lot about distributed systems, even if only in a simulated environment.


(*) I also reviewed the README content with ChatGPT before publishing it.

## How to use it

Compile:

(I'm using gcc v11.4.0)

```
g++ -o server server.cpp -lpthread
```

```
g++ -o client client.cpp
```

Run servers:

```
./server 127.0.0.3
./server 127.0.0.4
./server 127.0.0.4
```

Run clients:

```
./client
```

Quick demo:


https://github.com/user-attachments/assets/101e7897-c0b6-443e-a86c-2118466ff27a
