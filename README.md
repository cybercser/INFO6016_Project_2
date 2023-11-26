# INFO6016_Project_2: Authentication Service

## Overview

This project demonstrates a simple chat room application. It includes features such as account creation and authentication.

## Getting Started

### Database Setup

1. Ensure a MySQL database instance is running at `127.0.0.1:3306`. The auth server will connect to this database with the username 'root' and password 'root'.
2. Import the database by executing `chat_user.sql` and `chat_web_auth.sql`.

### Building the Project

1. Open the `INFO6016_Project_2.sln` solution file with Visual Studio.
2. Build the `ChatClient`, `ChatServer`, and `AuthServer` projects using the "Release x64" configuration.

### Running the Application

1. Navigate to the `x64/Release` directory in the project folder.
2. Start the auth server by running `AuthServer.exe`. The auth server will connect to the MySQL database running at `127.0.0.1:3306` using the username 'root' and password 'root'. It runs at `127.0.0.1:5556` and must be started first to allow the chat server to connect to it.
3. In the same directory, start the chat server by running `ChatServer.exe`. The chat server will automatically connect to the auth server and runs at `127.0.0.1:5555`.
4. Start a client by running `ChatClient.exe alice@gmail.com 123456789`. Here, 'alice@gmail.com' is the email (username), and '123456789' is the password. Note: The password length must be at least 8 characters.
5. Execute the test steps defined in `client_main.cpp` by pressing the number keys 0 and 1. Press '0' to create account, '1' to authenticate account.

## Features

This project demonstrates the following features:

- Account creation with validation. Errors are shown for invalid passwords (less than 8 characters) and existing accounts.
- Account authentication with error handling for invalid passwords and non-existent accounts.

For a visual demonstration of these features, please refer to `video.mp4`.