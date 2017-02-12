# chat-so-unict-l8

Chat app developed as a project for OS course at University of Catania.

The app is built using Linux sockets in the Internet Domain, thus in order to work correctly it needs the following files:

1. Server App (server_chat.c)

2. Client App (client_chat.c)

##1 - Server App

This app is the core of the whole system. It is used to manage all the incoming/outcoming messages from/to the clients. It also provides all the services used by the user running the `Client App`. 

###1.1 - General description

The app has been designed to realize a chat system where users can choose two different enviroments: Public room and Private room. Both the enviroments are implemented using a simple queue data structure. The queue is managed by the client thread which sends or receives messages for the `Client App`.

Moreover the single user can connect to it just using simple information such as the Server IP Address and a username. It is not required to create an account for accessing the system but the user has to choose a not used username. The username check is done dinamically looking for the connected clients user's.

The `Server App` sets a password which can be used by the moderator users.

There is a maximum number of connected user that is 128.

###1.2 - User's connection managment

When a client tries to establish a connection with the server, `Server App` creates a thread which will be used during the entire session for managing all the services for the user. 

It also fills a `struct` with the useful information for that user. There is an important field among all these information that is a `pointer` to the current queue; the pointer is dinamically changed when the user tries to switch from a room to another.

###1.3 - Chat rooms

The default chat room is the `Public room` which users connect to when a connection to the server is established. Using a particular command they can switch from Public to `Private rooms`. There are 8 private rooms created at server startup.

All the rooms (public and privates) are managed using the same algorithm. 

When a user leaves a private room comes back to the Public room.

###1.4 - Moderator mode

Moderator mode is accessible using a specific command and a given password. It allows the user to send messages in broadcast mode; the  broadcast message is sent just to the 8 private rooms.

When a user is in Moderator mode the system adds some keywords to its messages.

###1.5 - Messages managment

Reading and writing operations for messages managment is done using threads. The client`s main thread receives messages from the client and sends them to the current queue. Another thread instead is used to get messages from the queue and send them to the client.

###1.6 - Commands

List and description of the user commands:

- `/help`	List of the user commands

- `/quit`	Leaves the chat and closes `Client app`

- `/privatein`	Enters Private room

- `/privateout`	Leaves private room and enters Public room

- `/connessi`	List of the connected users in a specific room

- `/moderatore`	Enters the Moderator mode

- `/broadcast`	Sends a message in Broadcast mode

##2 - Client App

This app is used by a user for connecting to the `Server app` and use the services provided by it.

###2.1 - General description

Client app provides the user with an interface for reading and writing messages in the chat app. It works through two threads as explained below.

###2.2 - The two threads

Main thread sets up the client enviroment when `Client app` starts to execute. 

After network setup it asks user for providing information such as Server IP address and a username.

When the connection is established it is used for getting messages from the server.

Another thread is used for receiving messages from the user and send them to the `Server App`.
