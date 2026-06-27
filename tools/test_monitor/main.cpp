#include <raylib.h>
#include <cstdio>
#include <string>
#include <deque>
#include <cstring>
static constexpr const char* STATUS_FILE = "/tmp/theword_test_status.txt";
static constexpr int MAX_HISTORY = 10;

struct Entry {
    std::string text;
    std::string status; // "PASS", "FAIL"
};

static bool ReadStatusFile(std::string& outStatus, std::string& outText) {
    FILE* f = fopen(STATUS_FILE, "r");
    if (!f) return false;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        size_t len = std::strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (std::strncmp(line, "status:", 7) == 0)
            outStatus = line + 7;
        else if (std::strncmp(line, "text:", 5) == 0)
            outText = line + 5;
    }
    fclose(f);
    return true;
}

int main() {
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(480, 120, "TheWord Test Monitor");
    SetTargetFPS(8);

    // Position at top-center of the screen
    int monitorW = GetMonitorWidth(GetCurrentMonitor());
    SetWindowPosition(monitorW / 2 - 240, 0);

    std::deque<Entry> history;
    std::string curText, curStatus, prevText;
    float pollTimer = 0;
    const float POLL_INTERVAL = 0.2f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        pollTimer += dt;
        if (pollTimer >= POLL_INTERVAL) {
            pollTimer = 0;
            std::string newStatus, newText;
            if (ReadStatusFile(newStatus, newText)) {
                curStatus = newStatus;
                curText = newText;
                if (!curText.empty() && curText != prevText) {
                    if (!prevText.empty())
                        history.push_front({prevText, curStatus});
                    if (history.size() > MAX_HISTORY)
                        history.pop_back();
                    prevText = curText;
                }
            }
        }

        BeginDrawing();
        ClearBackground((Color){25, 25, 30, 255});

        // Current step
        Color curColor;
        const char* prefix;
        if (curStatus == "PASS") {
            curColor = (Color){80, 220, 100, 255}; prefix = "OK";
        } else if (curStatus == "FAIL") {
            curColor = (Color){240, 80, 80, 255}; prefix = "FAIL";
        } else {
            curColor = (Color){230, 200, 60, 255}; prefix = ">";
        }

        if (!curText.empty()) {
            std::string cur = std::string(prefix) + "  " + curText;
            DrawText(cur.c_str(), 14, 10, 20, curColor);
        } else {
            DrawText("Waiting for test...", 14, 10, 18, (Color){100, 100, 110, 255});
        }

        // Separator
        int sepY = 38;
        DrawLine(10, sepY, GetScreenWidth() - 10, sepY, (Color){50, 50, 58, 255});

        // History
        float histY = sepY + 6;
        float histSize = 14;
        for (const auto& e : history) {
            Color c;
            const char* p;
            if (e.status == "PASS") {
                c = (Color){100, 200, 120, 180}; p = "OK";
            } else if (e.status == "FAIL") {
                c = (Color){220, 120, 120, 180}; p = "FAIL";
            } else {
                c = (Color){180, 170, 100, 180}; p = "> ";
            }
            std::string line = std::string(p) + "  " + e.text;
            DrawText(line.c_str(), 16, (int)histY, (int)histSize, c);
            histY += histSize + 3;
            if (histY > GetScreenHeight() - 4) break;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
