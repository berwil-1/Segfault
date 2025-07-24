#pragma once
#include <fcntl.h>
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
start_process(const char * path, const std::vector<std::string> & args);
void
write_to_process(int fd, const std::string & input, bool done = false);
std::string
read_from_process(int fd);
void
close_process(const Process & proc);
