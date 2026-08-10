#include "StubChapterProvider.h"
#include <sstream>

namespace theword::data {
namespace {
std::vector<Word> TokenizeText(const std::string& text, int verseId, int& nextId) {
    std::vector<Word> words;
    std::istringstream stream(text);
    std::string word;
    while (stream >> word) {
        Word w;
        w.id = nextId++;
        w.verseId = verseId;
        w.text = word;
        words.push_back(w);
    }
    return words;
}
} // namespace

const char* StubChapterProvider::ProviderName() const {
    return "Stub";
}

std::optional<ChapterData> StubChapterProvider::LoadChapter(
        const std::string& bookId, int chapter) {
    if (bookId != "JHN" || chapter != 3) {
        return std::nullopt;
    }

    ChapterData data;
    data.bookId = "JHN";
    data.chapterNum = 3;

    const char* verse16 = "For God so loved the world that he gave his one and only Son that whoever believes in him shall not perish but have eternal life";
    const char* verse17 = "For God did not send his Son into the world to condemn the world but to save the world through him";
    const char* verse18 = "Whoever believes in him is not condemned but whoever does not believe stands condemned already because they have not believed in the name of Gods one and only Son";

    int nextId = 0;
    auto w16 = TokenizeText(verse16, 16, nextId);
    auto w17 = TokenizeText(verse17, 17, nextId);
    auto w18 = TokenizeText(verse18, 18, nextId);

    data.words.reserve(w16.size() + w17.size() + w18.size());
    data.words.insert(data.words.end(), w16.begin(), w16.end());
    data.words.insert(data.words.end(), w17.begin(), w17.end());
    data.words.insert(data.words.end(), w18.begin(), w18.end());

    Segment seg;
    seg.type = SegmentType::VerseText;
    seg.level = 0;
    seg.verseStart = 16;
    seg.verseEnd = 18;
    seg.startWordIndex = 0;
    seg.wordCount = data.words.size();
    data.segments.push_back(seg);

    return data;
}

} // namespace theword::data
