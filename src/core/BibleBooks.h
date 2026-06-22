#ifndef BibleBooks_h
#define BibleBooks_h

#include <string>
#include <array>
#include <cstdlib>

struct BookInfo {
    const char* code;
    const char* fullName;
    int chapterCount;
};

constexpr std::array<BookInfo, 66> BOOKS = {{
    {"GEN", "Genesis", 50},
    {"EXO", "Exodus", 40},
    {"LEV", "Leviticus", 27},
    {"NUM", "Numbers", 36},
    {"DEU", "Deuteronomy", 34},
    {"JOS", "Joshua", 24},
    {"JDG", "Judges", 21},
    {"RUT", "Ruth", 4},
    {"1SA", "1 Samuel", 31},
    {"2SA", "2 Samuel", 24},
    {"1KI", "1 Kings", 22},
    {"2KI", "2 Kings", 25},
    {"1CH", "1 Chronicles", 29},
    {"2CH", "2 Chronicles", 36},
    {"EZR", "Ezra", 10},
    {"NEH", "Nehemiah", 13},
    {"EST", "Esther", 10},
    {"JOB", "Job", 42},
    {"PSA", "Psalms", 150},
    {"PRO", "Proverbs", 31},
    {"ECC", "Ecclesiastes", 12},
    {"SNG", "Song of Solomon", 8},
    {"ISA", "Isaiah", 66},
    {"JER", "Jeremiah", 52},
    {"LAM", "Lamentations", 5},
    {"EZK", "Ezekiel", 48},
    {"DAN", "Daniel", 12},
    {"HOS", "Hosea", 14},
    {"JOL", "Joel", 3},
    {"AMO", "Amos", 9},
    {"OBA", "Obadiah", 1},
    {"JON", "Jonah", 4},
    {"MIC", "Micah", 7},
    {"NAM", "Nahum", 3},
    {"HAB", "Habakkuk", 3},
    {"ZEP", "Zephaniah", 3},
    {"HAG", "Haggai", 2},
    {"ZEC", "Zechariah", 14},
    {"MAL", "Malachi", 4},
    {"MAT", "Matthew", 28},
    {"MRK", "Mark", 16},
    {"LUK", "Luke", 24},
    {"JHN", "John", 21},
    {"ACT", "Acts", 28},
    {"ROM", "Romans", 16},
    {"1CO", "1 Corinthians", 16},
    {"2CO", "2 Corinthians", 13},
    {"GAL", "Galatians", 6},
    {"EPH", "Ephesians", 6},
    {"PHP", "Philippians", 4},
    {"COL", "Colossians", 4},
    {"1TH", "1 Thessalonians", 5},
    {"2TH", "2 Thessalonians", 3},
    {"1TI", "1 Timothy", 6},
    {"2TI", "2 Timothy", 4},
    {"TIT", "Titus", 3},
    {"PHM", "Philemon", 1},
    {"HEB", "Hebrews", 13},
    {"JAS", "James", 5},
    {"1PE", "1 Peter", 5},
    {"2PE", "2 Peter", 3},
    {"1JN", "1 John", 5},
    {"2JN", "2 John", 1},
    {"3JN", "3 John", 1},
    {"JUD", "Jude", 1},
    {"REV", "Revelation", 22}
}};

inline int FindBookIndex(const std::string& code) {
    for (size_t i = 0; i < BOOKS.size(); ++i) {
        if (code == BOOKS[i].code) return static_cast<int>(i);
    }
    return -1;
}

inline bool ParseChapterRef(const std::string& ref, std::string& book, int& chapter) {
    size_t dot = ref.rfind('.');
    if (dot == std::string::npos) return false;
    book = ref.substr(0, dot);
    chapter = std::stoi(ref.substr(dot + 1));
    return true;
}

inline std::string GetPreviousChapter(const std::string& current) {
    if (current.empty()) return "GEN.1";
    std::string book;
    int chapter;
    if (!ParseChapterRef(current, book, chapter)) return current;
    if (chapter > 1) return book + "." + std::to_string(chapter - 1);
    int idx = FindBookIndex(book);
    if (idx <= 0) return "";
    int prevIdx = idx - 1;
    return std::string(BOOKS[prevIdx].code) + "." + std::to_string(BOOKS[prevIdx].chapterCount);
}

inline std::string GetNextChapter(const std::string& current) {
    if (current.empty()) return "GEN.1";
    std::string book;
    int chapter;
    if (!ParseChapterRef(current, book, chapter)) return current;
    int idx = FindBookIndex(book);
    if (idx < 0) return current;
    if (chapter < BOOKS[idx].chapterCount) return book + "." + std::to_string(chapter + 1);
    if (idx >= static_cast<int>(BOOKS.size()) - 1) return "";
    int nextIdx = idx + 1;
    return std::string(BOOKS[nextIdx].code) + ".1";
}

inline std::string ChapterIdToTitle(const std::string& id) {
    std::string book;
    int chapter;
    if (!ParseChapterRef(id, book, chapter)) return id;
    int idx = FindBookIndex(book);
    if (idx < 0) return id;
    return std::string(BOOKS[idx].fullName) + " " + std::to_string(chapter);
}

#endif
