# Yerelnetwork
A C++ program which is able to scan your local network and connecting sending files and chatting features.

It uses async asio methods instead of synchronous methods. So there isn't extra threads running on your cpu.
It is much cleaner code than you expect from a random project.
It logs all the debug text to a file in %temp%.


- /list for listing all the devices on your local network.

- /file (username):(file_path) for sending a file to a specific user.

- (username):(message) for sending a message to a specific user.

Ports which being used:\
Device scaninng(UDP): 8888\
Main TCP: 9999\
File TCP: 9998

https://github.com/chriskohlhoff/asio project is heavily used.
