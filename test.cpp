#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <csignal>
#include <atomic>
#include <sys/types.h>
#include <sys/stat.h>
#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

// Global flag for graceful shutdown
std::atomic<bool> running{true};

void signal_handler(int signum) {
    running = false;
}

// Global descriptors for the pipes
int cmd_fd = -1;
int data_fd = -1;
std::string pipe_dir;

void cleanup() {
    // Tell the simulator to shut down [cite: 50, 53]
    if (cmd_fd != -1) {
        write(cmd_fd, "QUIT\n", 5);
        close(cmd_fd);
    }
    if (data_fd != -1) close(data_fd);
    
    // Delete the temp folder and pipes [cite: 25]
    if (!pipe_dir.empty()) {
        fs::remove_all(pipe_dir);
    }
}

int main(int argc, char* argv[]) {
    // Check if we have enough arguments [cite: 7, 8]
    if (argc < 4) {
        std::cerr << "Usage: ./client <world> <bug1> <bug2> [ticks] [fps]" << std::endl;
        return 1;
    }

    // Set up sim parameters or use defaults [cite: 8]
    std::string world = argv[1];
    std::string red_bug = argv[2];
    std::string black_bug = argv[3];
    int ticks = (argc > 4) ? std::stoi(argv[4]) : 50;
    int fps = (argc > 5) ? std::stoi(argv[5]) : 10;

    // Create a temp directory for our FIFOs [cite: 18]
    pipe_dir = "/tmp/bugworld_" + std::to_string(getpid());
    fs::create_directories(pipe_dir);
    
    std::string cmd_path = pipe_dir + "/cmd.pipe";
    std::string data_path = pipe_dir + "/data.pipe";

    mkfifo(cmd_path.c_str(), 0666);
    mkfifo(data_path.c_str(), 0666);

    // Fork and launch the simulator [cite: 19, 28]
    pid_t pid = fork();
    if (pid == 0) {
        execl("./sim", "./sim", "--cmd-pipe", cmd_path.c_str(), "--data-pipe", data_path.c_str(), 
              world.c_str(), red_bug.c_str(), black_bug.c_str(), NULL);
        return 1;
    }

    // Wait a bit for the sim to start [cite: 44]
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Open pipes in the order required by the protocol [cite: 33, 38-43]
    cmd_fd = open(cmd_path.c_str(), O_WRONLY);
    data_fd = open(data_path.c_str(), O_RDONLY | O_NONBLOCK);
    
    // Register signal handler for clean shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Simulation loop
    while (running) {
        // Clear screen for better display [cite: 24]
        system("clear");

        // Send step command [cite: 50]
        std::string cmd = "STEP " + std::to_string(ticks) + "\n";
        write(cmd_fd, cmd.c_str(), cmd.length());

        // Read the response until we hit the END marker [cite: 55, 105]
        std::string buffer;
        char chunk[1024];
        while (buffer.find("END") == std::string::npos) {
            ssize_t n = read(data_fd, chunk, sizeof(chunk) - 1);
            if (n > 0) {
                chunk[n] = '\0';
                buffer += chunk;
            } else if (n == 0) {
                std::cerr << "\n[Error] Simulator disconnected unexpectedly (EOF).\n";
                cleanup();
                return 1;
            } else if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // wait for data
                } else {
                    std::cerr << "\n[Error] Failed to read from sim data pipe.\n";
                    cleanup();
                    return 1;
                }
            }
        }

        // Print the current state to terminal [cite: 56-61]
        std::istringstream iss(buffer);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.rfind("CYCLE", 0) == 0 || line.rfind("STATS", 0) == 0 || line.rfind("MAP", 0) == 0 || line.rfind("END", 0) == 0) {
                std::cout << "\033[1;37m" << line << "\033[0m\n";
            } else if (line.rfind("ROW", 0) == 0) {
                // If the ROW has an extra space after "ROW ", it's staggered
                bool stagger = (line.length() > 4 && line[4] == ' ');
                if (stagger) std::cout << " ";
                
                size_t start_idx = 4 + (stagger ? 1 : 0);
                for (size_t i = start_idx; i < line.length(); ++i) {
                    char c = line[i];
                    if (c == ' ') std::cout << " ";
                    else if (c == 'R' || c == 'r') std::cout << "\033[1;31m" << c << "\033[0m"; // Red
                    else if (c == 'B' || c == 'b') std::cout << "\033[1;34m" << c << "\033[0m"; // Blue (Black bug)
                    else if (c == '#') std::cout << "\033[1;30m█\033[0m"; // Dark grey block for rocks
                    else if (c == '+') std::cout << "\033[1;41;37m+\033[0m"; // Red background nest
                    else if (c == '-') std::cout << "\033[1;44;37m-\033[0m"; // Blue background nest
                    else if (c >= '1' && c <= '9') std::cout << "\033[1;33m" << c << "\033[0m"; // Yellow food
                    else if (c == '.') std::cout << "\033[1;30m.\033[0m"; // Dark grey dot for empty
                    else std::cout << c;
                }
                std::cout << "\n";
            } else {
                std::cout << line << "\n";
            }
        }

        // Control simulation speed [cite: 8]
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
    }

    cleanup();
    return 0;
}