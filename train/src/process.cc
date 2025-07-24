#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

struct Process {
    pid_t pid;
    int   stdin_fd;
    int   stdout_fd;
};

Process
start_process(const char * path, const std::vector<std::string> & args) {
    int in_pipe[2], out_pipe[2];
    if (pipe(in_pipe) || pipe(out_pipe))
        throw std::runtime_error("pipe() failed");

    pid_t pid = fork();
    if (pid < 0)
        throw std::runtime_error("fork() failed");

    if (pid == 0) {
        // Child process
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);

        // Close all pipe ends after dup
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);

        // Disable buffering
        setvbuf(stdout, nullptr, _IONBF, 0);

        std::vector<char *> exec_args;
        exec_args.push_back(const_cast<char *>(path));
        for (const auto & arg : args)
            exec_args.push_back(const_cast<char *>(arg.c_str()));
        exec_args.push_back(nullptr);

        execvp(path, exec_args.data());
        _exit(1); // Only reached if exec fails
    }

    // Parent process
    close(in_pipe[0]); // parent doesn't read from stdin
    close(out_pipe[1]); // parent doesn't write to stdout

    return Process{pid, in_pipe[1], out_pipe[0]};
}

void
write_to_process(int fd, const std::string & input, bool done) {
    write(fd, input.c_str(), input.size());
    if (done)
        close(fd);
}

std::string
read_from_process(int fd) {
    std::string output;
    char        buffer[256];
    ssize_t     n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        output.append(buffer, n);
    }
    close(fd);
    return output;
}

void
close_process(const Process & proc) {
    int status;
    waitpid(proc.pid, &status, 0);
}
