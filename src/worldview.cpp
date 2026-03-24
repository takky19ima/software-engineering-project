#include "worldview.h"
#include <QPainter>
#include <QImage>
#include <QDebug>

// constructor
WorldView::WorldView(QWidget *parent)
    : QWidget(parent)
{
}

// store bug data and request redraw
void WorldView::setBugs(const std::vector<Bug>& b)
{
    bugs = b;
    update(); // triggers paintEvent
}

void WorldView::setMap(const std::vector<std::string>& map, const std::vector<bool>& offsets)
{
    if (map.empty()) return;

    // 1️⃣ Init traceMap first
    if (traceMap.empty() || traceMap.size() != map.size() || traceMap[0].size() != map[0].size()) {
        traceMap.assign(map.size(), std::vector<int>(map[0].size(), 0));
    }

    // 2️⃣ Mark traces FIRST (before fading)
    if (!previousMap.empty())
    {
        for (int r = 0; r < (int)previousMap.size(); ++r)
        {
            for (int c = 0; c < (int)previousMap[r].size(); ++c)
            {
                char oldCh = previousMap[r][c];
                char newCh = map[r][c];

                if ((oldCh == 'R' || oldCh == 'r' || oldCh == 'B' || oldCh == 'b') &&
    !(newCh == 'R' || newCh == 'r' || newCh == 'B' || newCh == 'b'))
{
    traceMap[r][c] = traceLength;
}
            }
        }
    }

    // 3️⃣ Fade — skip cells we just marked this frame (value == traceLength)
    for (int r = 0; r < (int)traceMap.size(); ++r)
        for (int c = 0; c < (int)traceMap[r].size(); ++c)
            if (traceMap[r][c] > 0 && traceMap[r][c] < traceLength)
                traceMap[r][c]--;

    previousMap = map;
    currentMap  = map;
    offsetRows = offsets;

    update();
}

void WorldView::paintEvent(QPaintEvent *)
{
    int rows = currentMap.size();
    int cols = rows ? currentMap[0].size() : 0;

    // ── Render to QImage first (fixes X11 forwarding) ──
    QImage image(size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(160, 160, 160));  // gray background

    if (rows == 0 || cols == 0) {
        QPainter widgetPainter(this);
        widgetPainter.drawImage(0, 0, image);
        return;
    }

    int cellSize = std::min(width() / (cols + 1), height() / rows);
    if (cellSize < 2) cellSize = 2;

    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing, true);

    // ── Pass 1: draw terrain ──
    for (int r = 0; r < rows; ++r)
    {
        int xOff = (r < (int)offsetRows.size() && offsetRows[r]) ? cellSize / 2 : 0;

        for (int c = 0; c < (int)currentMap[r].size(); ++c)
        {
            char ch = currentMap[r][c];
            QRect rect(c * cellSize + xOff, r * cellSize, cellSize, cellSize);

            // base terrain fill
            QColor fill;
            switch (ch)
            {
                case '#':  fill = QColor(100, 100, 100); break;      // walls — dark gray
                case '.':  fill = QColor(210, 210, 200); break;      // empty — light
                case '+':  fill = QColor(255, 180, 180); break;      // red base — pink
                case '-':  fill = QColor(180, 230, 180); break;      // black base — green
                default:   fill = QColor(210, 210, 200); break;      // fallback
            }
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            p.drawRect(rect);

            // cell outline
            p.setPen(QPen(QColor(180, 180, 180), 1));
            p.setBrush(Qt::NoBrush);
            p.drawRect(rect);

            // ── Draw bugs ──
            if (ch == 'R' || ch == 'r') {
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(200, 30, 30));  // red bug
                p.drawEllipse(rect.adjusted(2, 2, -2, -2));
                if (ch == 'r') {  // carrying food
                    p.setBrush(QColor(255, 220, 0));
                    int d = cellSize / 4;
                    p.drawEllipse(rect.adjusted(d, d, -d, -d));
                }
            }
            else if (ch == 'B' || ch == 'b') {
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(30, 30, 30));  // black bug
                p.drawEllipse(rect.adjusted(2, 2, -2, -2));
                if (ch == 'b') {  // carrying food
                    p.setBrush(QColor(255, 220, 0));
                    int d = cellSize / 4;
                    p.drawEllipse(rect.adjusted(d, d, -d, -d));
                }
            }
            // ── Draw food ──
            else if (ch >= '1' && ch <= '9') {
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(240, 200, 0));  // yellow food
                p.drawEllipse(rect.adjusted(3, 3, -3, -3));
            }
        }
    }

    // ── Pass 2: trace overlay ──
    for (int r = 0; r < rows; ++r)
    {
        int xOff = (r < (int)offsetRows.size() && offsetRows[r]) ? cellSize / 2 : 0;

        for (int c = 0; c < (int)traceMap[r].size(); ++c)
        {
            if (traceMap[r][c] > 0)
            {
                QRect rect(c * cellSize + xOff, r * cellSize, cellSize, cellSize);
                int alpha = (traceLength > 0) ? 200 * traceMap[r][c] / traceLength : 0;
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(255, 140, 0, alpha));
                p.drawRect(rect);
            }
        }
    }

    p.end();

    // ── Blit the image to the widget ──
    QPainter widgetPainter(this);
    widgetPainter.drawImage(0, 0, image);
}