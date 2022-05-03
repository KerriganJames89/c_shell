# FSU COP4610 Project 1
- The goal of this project is to implement a simple shell using C, that supports basic I/O, external commands, piping, redirection, etc.

# Team Members
- Sam Anderson 
- Luis Corps
- James Kerrigan

# File listing
Our project multiple contains files:
- main.c 	: source code primary.
- makefile 	: directive set.
- README 	: contains information regarding the project in detail.

# Makefile description
- Our makefile has a single dependency `main.o`.
- How to run how program:
    1) type `make` which will create an executable called `shell.x`
    2) then type `./shell.x` to run our project executable

# Group Member Contribution
- **Sam Anderson**
    - Created github repo
    - Created makefile
    - Worked on `Part 2`: replaced tokens beginning with $ to there corresponding values
    - Worked on `Part 3`: added the prompt 
    - Worked on `Part 4`: tilde expansion
    - Worked on `Part 5`: implemented valid_externalcmd () function which checks for the existence and location of external commands 
    - Worked on `Part 6`: Can execute external commands in a background process such as; ls, cat, head, etc.
    - Worked on `Part 9`: Background Processing shell can perform background processing with piping 
    - Worked on `Part 10`: implemented the cd command, echo command, and exit command (total run time and longest running process)

- **Luis Corps**
	- Contributed to main function, specifically, use cases involving piping
    - Worked on `Part 7`: minor implementation and research
	- Worked on `Part 8`: implemented pipe_command function
    - Worked on `Extra Credit`: re-implemented Part 8 to support unlimited number of pipes 
    - Worked on `Extra Credit`: re-implemented Part 8 to support piping and I/O redirection in a single command
    
- **James Kerrigan**
    - Worked on `Part 4`: Minor pseudocode.
    - Worked on `Part 5`: Research and minor pseudocode.
    - Worked on `Part 6`: Contributions to the implementation of external commands; see part 7.
    - Worked on `Part 7`: I/O redirection implementation.
    - Worked on `Part 9`: Added list functionality for background processes using TAILQ().
    - Worked on `Part 10`: Modifications to the echo command. List cleanup.
 
# Bugs
- Runtime: No whitespace inbetween tokens will produce errors. As the tokenizer was provided, we made no change to it.
- Runtime: Few conflicts when doing incorrect commands using specific operators; "<", ">", and "|". Fixed a few occurences by error checking, but 
  ultimately requires some adjustment to the tokenizer.
- Runtime: Jobs don't release from the queue until user is prompt again. This was by design to display the job results as required.

# Extra Credit
- Shell-ception: can execute your shell from within a running shell process repeatedly
- Support unlimited number of pipes.
- Support piping and I/O redirection in a single command.  

# Special considerations
- Some of the git log may not accurately represent each group members contribution, this is because we made use of discord
to sent code snippets too group members and a VS Code extension called `Live Share`.
