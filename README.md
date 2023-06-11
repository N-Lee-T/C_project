# C project

## This project creates a shell using (you maybe guessed this) the C language. 

Since I coded this very soon after working with C for the very first time, it's quite rough - but it works! It was also quite rewarding, since my previous coding experience was entirely in Python and SQL, with a little bit a Javascript thrown in there. It was fun to look through the Man Pages and find new functions and learn about Linux. Having to extensively research the documentation was a very valuable lesson, and something that I wish I had done when I was first learning Python. It's a great way to learn a language, and a terrific way to start understand what goes on 'under the hood' with Python.

That said, I likely won't be taking up C any further. My primary goal is to learn skills that I can apply with regularity to a new career, and learning one of C#, C++, Java, or Go are higher priorities.

## The basics

The shell works the same as... a shell. It implements basic functionality: 
- Expanding the $$, $?, $!, and ~ tokens
- Custom behavior for handling signals (SIGINT, SIGSTOP)
- Implementation of I/O redirection (<, >) and background (&) operators
- Implementation of *exit* and *cd*
- Calling *exec* waiting where appropriate 

## Summary
The shell displays a prompt string and waits for a command. For built-in commands *exit* and *cd*, as well as *ls*, it simply executes; for others, it calls *exec*. All commands, as usual, will be run in a child process. 

If '&' is present at the end of the command it runs the command in the background and re-prints the prompt string, and otherwise it performs a blocking wait, performing the action and then displaying the prompt string after process termination. At a call of *exit*, all child processes are terminated.



Yes, there is a terrible memory leak.










