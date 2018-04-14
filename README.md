# Simple-Shell
A simple linux shell that takes in arguments and runs the specified commands

This project was built for CMPT 300 - Operating Systems I

Commands that were implemented manually include:

- exit
- pwd
- cd
- history

The user can also enter `!` or send a SIGINT `CTRL-C` to display the history, or `!n` to repeat the nth command, where `n` is a number less than the total number of commands typed.

All other commands enetered fork a child process and run `execvp()`.

The parent process will wait for the child to finish. The user can also enter the symbol `&` as their last token to have the shell loop back to read another command immediately, without waiting for the child process.
