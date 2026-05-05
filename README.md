# Custom Unix Shell

A lightweight Unix-like shell implemented in C, supporting basic command execution, redirection, pipelining, process management, and inter-process communication features.

## 🚀 Features
- **Command Execution**: Execute standard system programs with arguments.
- **Pipes (`|`)**: Support for connecting the output of one command to the input of another.
- **I/O Redirection**:
  - `<`: Input redirection from a file.
  - `>`: Output redirection to a file (overwrite).
  - `>>`: Output redirection to a file (append).
- **Built-in Commands**: Includes support for changing directories (`chdir`).
- **Interactive Prompt**: Displays the current working directory and user context.

## 🛠️ Technical Details
- Implements process creation using `fork()` and `execvp()`.
- Manages inter-process communication via `pipe()` and `dup2()`.
- Handles file operations using the standard Unix file API.

## ⚙️ Compilation & Usage
### Building
Run the following command in the project root:
```bash
make
```

### Running
Launch the shell:
```bash
./sashell
```

---
*Developed by Sarantis Sarantinos.*
