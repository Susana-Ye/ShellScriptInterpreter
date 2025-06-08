<h1>🖥️ Scripter: Linux Shell Script Interpreter [C Project]</h1>

 ### [YouTube Demo & Output Samples]()

<h2>📝 Description</h2>
Scripter is a command interpreter written in C for Linux, designed to emulate core functionalities of a shell as part of an Operating Systems lab. It reads a script file line-by-line, interprets and executes each command using POSIX system calls. The project reinforces understanding of process management, file descriptor manipulation, piping, I/O redirection, and background execution.
<br><br>
The interpreter also supports an external command (mygrep), developed as a C program that mimics grep functionality, searching for strings within files.
<br />


<h2>🛠️ Languages and Utilities Used</h2>

- <b>C</b>
- <b>GCC</b> 
- <b>POSIX System Calls</b>
- <b>Linux</b>

<h2>📷 Output Examples</h2>

<p align="center">
Valid script execution: <br/>
<img src="https://i.imgur.com/example1.png" width="60%" alt="Scripter output"/>
<br /><br />
Redirection and piping example: <br/>
<img src="https://i.imgur.com/example2.png" width="60%" alt="Redirection output"/>
<br /><br />
Background command with PID output: <br/>
<img src="https://i.imgur.com/example3.png" width="60%" alt="Background PID"/>
<br /><br />
Mygrep execution: <br/>
<img src="https://i.imgur.com/example4.png" width="60%" alt="Mygrep output"/>
</p>

<h2>📌 Features</h2>

- Validates script header (`## Script de SSOO`) and format.
- Executes sequential Linux commands line-by-line.
- Handles:
   - Simple commands with arguments.
   - Piped commands.
   - Input (`<`), output (`>`), and error (`!>`) redirections.
   - Background execution (`&`) with proper PID printing and zombie management.
   - Launches external command `mygrep` with same redirection/background capabilities. 

<h2>📽️ Usage Instructions</h2>

1. Clone or Download the Project
2. Compile using Make:
```bash
make all       # builds scripter and the external 'mygrep' command
```
3. Execute with a script file:
```bash
./scripter example_script.txt    # the first line of script.txt must be '## Script de SSOO'
```
4. Clean binaries:
```bash
make clean      # cleans the build files
```

<h2>🧑‍💻 Authors</h2>

- Susana Ye Zhan
- Xinyi Yewu
```
This project is for educational purposes only. All rights reserved to Universidad Carlos III de Madrid.
```

<!--
 ```diff
- text in red
+ text in green
! text in orange
# text in gray
@@ text in purple (and bold)@@
```
--!>
