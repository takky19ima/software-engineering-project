#pragma once
#include <QWidget>
#include <vector>
#include "client.h"


// simple data structure
struct Bug {
    int x;
    int y;
    char type; // 'R' or 'B'
};

class WorldView : public QWidget
{
    Q_OBJECT

public:
    explicit WorldView(QWidget *parent = nullptr);

    void setBugs(const std::vector<Bug>& b);

    void setMap(const std::vector<std::string>& map, const std::vector<bool>& offsets);
    void setTraceLength(int n) { traceLength = n; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::vector<Bug> bugs;
    std::vector<std::string> currentMap; 
    std::vector<std::vector<int>> traceMap;
    std::vector<std::string> previousMap;
    int traceLength = 5;
        std::vector<bool> offsetRows;

};