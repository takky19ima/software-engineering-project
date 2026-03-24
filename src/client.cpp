#include "client.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <sstream>
#include <vector>

using namespace std;

Client::Client()
{
    cmd_fd = -1;
    data_fd = -1;
    sim_pid = -1;
}

Client::~Client()
{
    stop();
}

// ------------------------------------------------------------
// Start simulator and setup pipes
// ------------------------------------------------------------
bool Client::start(const string& world,
                   const string& redBug,
                   const string& blackBug)
{
    char temp_dir_template[] = "/tmp/bugworld_XXXXXX";
    temp_dir = mkdtemp(temp_dir_template);

    if (!temp_dir) {
        perror("mkdtemp failed");
        return false;
    }

    cmd_pipe = string(temp_dir) + "/cmd.pipe";
    data_pipe = string(temp_dir) + "/data.pipe";

    if (mkfifo(cmd_pipe.c_str(), 0666) == -1) {
        perror("mkfifo cmd.pipe failed");
        return false;
    }

    if (mkfifo(data_pipe.c_str(), 0666) == -1) {
        perror("mkfifo data.pipe failed");
        return false;
    }

    // Fork simulator
    sim_pid = fork();

    if (sim_pid < 0) {
        perror("fork failed");
        return false;
    }

    if (sim_pid == 0) {
        char* args[] = {
            (char*)"./sim",
            (char*)"--cmd-pipe",
            (char*)cmd_pipe.c_str(),
            (char*)"--data-pipe",
            (char*)data_pipe.c_str(),
            (char*)world.c_str(),
            (char*)redBug.c_str(),
            (char*)blackBug.c_str(),
            nullptr
        };

        execvp("./sim", args);
        perror("exec failed");
        exit(1);
    }

    // Give simulator time to start
    sleep(1);

    // Open data pipe first
    data_fd = open(data_pipe.c_str(), O_RDONLY | O_NONBLOCK);
    if (data_fd == -1) {
        perror("open data.pipe failed");
        return false;
    }

    // Retry opening command pipe until simulator connects
    while (true) {
        cmd_fd = open(cmd_pipe.c_str(), O_WRONLY | O_NONBLOCK);

        if (cmd_fd != -1)
            break;

        if (errno == ENXIO) {
            usleep(100000);
        } else {
            perror("open cmd.pipe failed");
            return false;
        }
    }

    return true;
}

// ------------------------------------------------------------
// Send STEP command and read simulator response
// ------------------------------------------------------------
string Client::step(int ticks
)
{
    if (cmd_fd == -1 || data_fd == -1)
        return "jkljj";

    string command = "STEP " + to_string(ticks) + "\n";

    if (write(cmd_fd, command.c_str(), command.size()) == -1) {
        perror("write STEP failed");
        return "";
    }

    char buffer[1024];
    string response;

    while (true) {
        ssize_t bytes_read = read(data_fd, buffer, sizeof(buffer) - 1);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            response += buffer;

            if (response.find("END") != string::npos)
                break;
        }
        else {
            // No data yet — wait briefly
            usleep(10000);
        }
    }

    return response;
}

// ------------------------------------------------------------
// Parse simulator response and update current map and stats
// ------------------------------------------------------------
void Client::parseResponse(const std::string& response) {
    currentMap.clear(); // Clear previous map
    offsetRows.clear(); // Clear previous offset tracking

    std::istringstream iss(response); //streaming from data which is provided by simulator
    std::string line;

    int cycle = 0;
    int row = 0, col = 0;
    int red_alive = 0, black_alive = 0, red_food = 0, black_food = 0;

    while (getline(iss, line)) {
        if (line.find("CYCLE") == 0) {
            sscanf(line.c_str(), "CYCLE %d", &cycle);
            
        } else if (line.find("MAP") == 0) {
            sscanf(line.c_str(), "MAP %d %d", &row, &col);

        } else if (line.find("ROW") == 0) {
            std::string rowData = line.substr(4);// Extract the part after "ROW"
            bool isOffset = (!rowData.empty() && rowData[0] == ' ');//row starts with " ": Ture or False
            offsetRows.push_back(isOffset);
            std::string cleaned;
            //remove the spaces inside worldmap for better UI window
            for (char ch : rowData) {
                if (ch != ' ') {
                    cleaned += ch;
                }
            }
            currentMap.push_back(cleaned);
        } else if (line.find("STATS") == 0) {
            sscanf(line.c_str(), "STATS %d %d %d %d", &red_alive, &black_alive, &red_food, &black_food);
        } else if (line.find("END") == 0) {
            break;
        
        }
    }
    cout << cycle << endl;
    for (const auto& row : currentMap) {
        cout << row << endl;
    }
    cout << red_alive << " " << black_alive << " " << red_food << " " << black_food << endl;
}

// Give access to currentmap  UI rendering
std::vector<std::string> Client::getCurrentMap() const {
    return currentMap;
}

// Give access to offset rows for UI rendering
std::vector<bool> Client::getOffsetRows() const {
    return offsetRows;
}

// ------------------------------------------------------------
// Stop simulator and cleanup
// ------------------------------------------------------------
void Client::stop()
{
    if (cmd_fd != -1) {
        string quit_cmd = "QUIT\n";
        write(cmd_fd, quit_cmd.c_str(), quit_cmd.size());
        close(cmd_fd);
        cmd_fd = -1;
    }

    if (data_fd != -1) {
        close(data_fd);
        data_fd = -1;
    }

    if (sim_pid > 0) {
        waitpid(sim_pid, nullptr, 0);
        sim_pid = -1;
    }

    if (!cmd_pipe.empty())
        unlink(cmd_pipe.c_str());

    if (!data_pipe.empty())
        unlink(data_pipe.c_str());

    if (temp_dir)
        rmdir(temp_dir);
}