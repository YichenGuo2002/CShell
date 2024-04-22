 - Assignment Name: C Command-line Interface

 - Assignment Tech Stack: C

 - Description: The shell is essentially a command-line interpreter. It works as follows:
    - It prompts you to enter a command.
    - It interprets the command you entered.
    - If you entered a built-in command (e.g., cd), then the shell runs that command.
    - If you entered an external program (e.g., /bin/ls), or multiple programs connected through pipes (e.g., ls -l | less), then the shell creates child processes, executes these programs, and waits for all these processes to either terminate or be suspended.
    - If you entered something wrong, then the shell prints an error message.
    - Rinse and repeat until you press Ctrl-D to close STDIN or enter the built-in command exit, at which point the shell exits.
   Here are some examples of valid commands:
    ```
    A blank line.
    /usr/bin/ls -a -l
    cat shell.c | grep main | less
    cat < input.txt
    cat > output.txt
    cat >> output.txt
    cat < input.txt > output.txt
    cat < input.txt >> output.txt
    cat > output.txt < input.txt
    cat >> output.txt < input.txt
    cat < input.txt | cat > output.txt
    cat < input.txt | cat | cat >> output.txt
    ```
    Here are some examples of invalid commands:
    ```
    cat <
    cat >
    cat |
    | cat
    cat << file.txt
    cat < file.txt < file2.txt
    cat < file.txt file2.txt
    cat > file.txt > file2.txt
    cat > file.txt >> file2.txt
    cat > file.txt file2.txt
    cat > file.txt | cat
    cat | cat < file.txt
    cd / > file.txt
    ```
