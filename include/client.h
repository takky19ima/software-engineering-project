#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <sys/types.h>
#include <vector>

class Client
{
public:
    Client();
    ~Client();

    bool start(const std::string& world,
               const std::string& redBug,
               const std::string& blackBug);
    
    void parseResponse(const std::string& response);
    std::vector<std::string> getCurrentMap() const;
    std::vector<bool> getOffsetRows() const;

    std::string step(int ticks);

    void stop();
    std::string getresult() const;

private:
    int cmd_fd;
    int data_fd;
    pid_t sim_pid;
    char* temp_dir;
    std::string cmd_pipe;
    std::string data_pipe;
    std::vector<std::string> currentMap; // contain each row of the world as string
    std::vector<bool> offsetRows; // track which rows have offsets
};
#endif