#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "core/Config.h"
#include "core/EnvLoader.h"
#include "core/BibleBooks.h"
#include "core/APIClient.h"
#include "data/ChapterProvider.h"
#include "data/BibleClient.h"
#include "data/CompositeProvider.h"
#include "data/StubChapterProvider.h"
#include "data/USFMParser.h"
#include "highlight/InMemoryStorage.h"
#include "highlight/Highlighter.h"
#include "persistence/PersistenceManager.h"

// Test helper: exposes private parseHtmlChapter for unit testing
class BibleClientTest {
public:
    static std::optional<ChapterData> Parse(BibleClient& client,
            const std::string& html, const std::string& bookId, int chapter) {
        return client.parseHtmlChapter(html, bookId, chapter);
    }
};

TEST_CASE("Config constants are defined") {
    CHECK(config::WINDOW_WIDTH == 450);
    CHECK(config::WINDOW_HEIGHT == 800);
    CHECK(config::TARGET_FPS == 60);
    CHECK_FALSE(std::string(config::FONT_REGULAR).empty());
    CHECK_FALSE(std::string(config::FONT_BOLD).empty());
    CHECK_FALSE(std::string(config::USFM_DIR).empty());
}

TEST_CASE("Config environment key constants") {
    CHECK_FALSE(std::string(config::ENV_FILE).empty());
    CHECK(config::YVP_APP_KEY == std::string("YVP_APP_KEY"));
}

TEST_CASE("EnvLoader returns empty for missing key") {
    std::string value = EnvLoader::get("NONEXISTENT_KEY_THAT_SHOULD_NOT_EXIST");
    CHECK(value.empty());
}

TEST_CASE("EnvLoader returns default for missing key") {
    std::string value = EnvLoader::get("NONEXISTENT_KEY", "fallback_value");
    CHECK(value == "fallback_value");
}

TEST_CASE("BibleBooks finds book by code") {
    int genIdx = FindBookIndex("GEN");
    CHECK(genIdx == 0);

    int jhnIdx = FindBookIndex("JHN");
    CHECK(jhnIdx == 42);

    int revIdx = FindBookIndex("REV");
    CHECK(revIdx == 65);

    int missing = FindBookIndex("NONEXISTENT");
    CHECK(missing == -1);
}

TEST_CASE("BibleBooks chapter navigation within book") {
    std::string prev = GetPreviousChapter("JHN.3");
    CHECK(prev == "JHN.2");

    std::string next = GetNextChapter("JHN.3");
    CHECK(next == "JHN.4");
}

TEST_CASE("BibleBooks chapter navigation across books") {
    std::string prevFromFirst = GetPreviousChapter("JHN.1");
    CHECK(prevFromFirst == "LUK.24");

    std::string nextFromLast = GetNextChapter("JHN.21");
    CHECK(nextFromLast == "ACT.1");
}

TEST_CASE("BibleBooks boundaries") {
    // First book, first chapter — no previous
    std::string beforeGenesis = GetPreviousChapter("GEN.1");
    CHECK(beforeGenesis.empty());

    // Last book, last chapter — no next
    std::string afterRevelation = GetNextChapter("REV.22");
    CHECK(afterRevelation.empty());
}

TEST_CASE("BibleBooks ParseChapterRef") {
    std::string book;
    int chapter;

    bool ok = ParseChapterRef("JHN.3", book, chapter);
    CHECK(ok);
    CHECK(book == "JHN");
    CHECK(chapter == 3);

    ok = ParseChapterRef("GEN.50", book, chapter);
    CHECK(ok);
    CHECK(book == "GEN");
    CHECK(chapter == 50);
}

TEST_CASE("BibleBooks ChapterIdToTitle") {
    std::string title = ChapterIdToTitle("JHN.3");
    CHECK(title == "John 3");

    title = ChapterIdToTitle("GEN.1");
    CHECK(title == "Genesis 1");
}

TEST_CASE("EnvLoader handles system environment") {
    // SYSTEMROOT is typically set on Windows, HOME on Unix
    std::string home = EnvLoader::get("HOME");
    if (!home.empty()) {
        CHECK_FALSE(home.empty());
    }
}

TEST_CASE("StubChapterProvider returns John 3") {
    StubChapterProvider stub;
    auto result = stub.LoadChapter("JHN", 3);
    CHECK(result.has_value());
    CHECK(result->bookId == "JHN");
    CHECK(result->chapterNum == 3);
    CHECK_FALSE(result->words.empty());
    CHECK_FALSE(result->segments.empty());
    CHECK(result->segments[0].type == SegmentType::VerseText);
}

TEST_CASE("StubChapterProvider returns nullopt for unknown chapter") {
    StubChapterProvider stub;
    CHECK_FALSE(stub.LoadChapter("GEN", 1).has_value());
    CHECK_FALSE(stub.LoadChapter("JHN", 1).has_value());
    CHECK_FALSE(stub.LoadChapter("JHN", 4).has_value());
}

TEST_CASE("StubChapterProvider HasChapter") {
    StubChapterProvider stub;
    CHECK(stub.HasChapter("JHN", 3));
    CHECK_FALSE(stub.HasChapter("GEN", 1));
    CHECK(stub.ProviderName() == std::string("Stub"));
}

TEST_CASE("ChapterData segment types") {
    StubChapterProvider stub;
    auto result = stub.LoadChapter("JHN", 3);
    REQUIRE(result.has_value());

    for (const auto& seg : result->segments) {
        CHECK(seg.wordCount > 0);
        CHECK(seg.verseStart >= 16);
        CHECK(seg.verseEnd >= 16);
        CHECK(seg.verseEnd >= seg.verseStart);
    }
}

TEST_CASE("Span carries SegmentType") {
    Span span;
    span.text = "test";
    span.type = SegmentType::VerseText;
    CHECK(span.type == SegmentType::VerseText);
    CHECK(span.text == "test");
}

TEST_CASE("Line groups spans") {
    Line line;
    line.y = 0.0f;
    line.height = 20.0f;

    Span s1;
    s1.text = "hello";
    s1.type = SegmentType::VerseText;
    line.spans.push_back(s1);

    Span s2;
    s2.text = "world";
    s2.type = SegmentType::VerseText;
    line.spans.push_back(s2);

    CHECK(line.spans.size() == 2);
    CHECK(line.spans[0].text == "hello");
    CHECK(line.spans[1].text == "world");
}

TEST_CASE("ChapterLayout stores lines") {
    ChapterLayout layout;
    layout.chapterId = "JHN.3";
    layout.totalHeight = 100.0f;

    Line line;
    line.y = 0.0f;
    line.height = 20.0f;
    layout.lines.push_back(line);

    CHECK(layout.lines.size() == 1);
    CHECK(layout.chapterId == "JHN.3");
    CHECK(layout.totalHeight == 100.0f);
}

TEST_CASE("USFMParser loads Genesis 1 from file") {
    USFMParser parser(config::USFM_DIR);
    auto result = parser.LoadChapter("GEN", 1);
    REQUIRE(result.has_value());
    CHECK(result->bookId == "GEN");
    CHECK(result->chapterNum == 1);
    CHECK_FALSE(result->words.empty());
    CHECK_FALSE(result->segments.empty());
}

TEST_CASE("USFMParser Genesis 1 has BookTitle segment") {
    USFMParser parser(config::USFM_DIR);
    auto result = parser.LoadChapter("GEN", 1);
    REQUIRE(result.has_value());

    bool hasBookTitle = false;
    bool hasChapterLabel = false;
    bool hasVerseText = false;
    for (const auto& seg : result->segments) {
        if (seg.type == SegmentType::BookTitle) hasBookTitle = true;
        if (seg.type == SegmentType::ChapterLabel) hasChapterLabel = true;
        if (seg.type == SegmentType::VerseText) hasVerseText = true;
    }
    CHECK(hasBookTitle);
    CHECK(hasChapterLabel);
    CHECK(hasVerseText);
}

TEST_CASE("USFMParser Genesis 1 verse content") {
    USFMParser parser(config::USFM_DIR);
    auto result = parser.LoadChapter("GEN", 1);
    REQUIRE(result.has_value());
    REQUIRE_FALSE(result->words.empty());

    // First word should be verse 1
    CHECK(result->words[0].verseId == 1);
    CHECK_FALSE(result->words[0].text.empty());
}

TEST_CASE("USFMParser HasChapter") {
    USFMParser parser(config::USFM_DIR);
    CHECK(parser.HasChapter("GEN", 1));
    CHECK(parser.HasChapter("GEN", 50));
    CHECK(parser.HasChapter("PSA", 150));
    CHECK(parser.HasChapter("REV", 22));
    CHECK_FALSE(parser.HasChapter("GEN", 999));
    CHECK_FALSE(parser.HasChapter("NONEXISTENT", 1));
}

TEST_CASE("USFMParser missing file returns nullopt") {
    USFMParser parser(config::USFM_DIR);
    auto result = parser.LoadChapter("NONEXISTENT", 1);
    CHECK_FALSE(result.has_value());
}

TEST_CASE("USFMParser ProviderName") {
    USFMParser parser(config::USFM_DIR);
    CHECK(std::string(parser.ProviderName()) == "USFMParser");
}

TEST_CASE("USFMParser loads multiple books") {
    USFMParser parser(config::USFM_DIR);
    auto gen1 = parser.LoadChapter("GEN", 1);
    auto exo1 = parser.LoadChapter("EXO", 1);
    auto rev22 = parser.LoadChapter("REV", 22);

    CHECK(gen1.has_value());
    CHECK(exo1.has_value());
    CHECK(rev22.has_value());
    CHECK(gen1->bookId == "GEN");
    CHECK(exo1->bookId == "EXO");
    CHECK(rev22->bookId == "REV");
}

TEST_CASE("USFMParser caches book data") {
    USFMParser parser(config::USFM_DIR);
    auto first = parser.LoadChapter("GEN", 1);
    auto second = parser.LoadChapter("GEN", 2);
    CHECK(first.has_value());
    CHECK(second.has_value());
    CHECK(first->chapterNum == 1);
    CHECK(second->chapterNum == 2);
}

// ── BibleClient HTML Parser Tests ──────────────────────────────────────

TEST_CASE("BibleClient parses section heading from HTML") {
    APIClient api;
    BibleClient client(api, 3034);
    std::string html = "<div class=\"s1 yv-h\">The Creation</div>";
    auto result = BibleClientTest::Parse(client, html, "GEN", 1);
    REQUIRE(result.has_value());
    CHECK(result->bookId == "GEN");
    CHECK(result->chapterNum == 1);
    REQUIRE_FALSE(result->segments.empty());
    CHECK(result->segments[0].type == SegmentType::SectionHeading);
    CHECK(result->segments[0].level == 1);
    CHECK(result->segments[0].text == "The Creation");
}

TEST_CASE("BibleClient parses s2 heading with different level") {
    APIClient api;
    BibleClient client(api, 3034);
    std::string html = "<div class=\"s2 yv-h\">Subheading</div>";
    auto result = BibleClientTest::Parse(client, html, "GEN", 2);
    REQUIRE(result.has_value());
    REQUIRE_FALSE(result->segments.empty());
    CHECK(result->segments[0].type == SegmentType::SectionHeading);
    CHECK(result->segments[0].level == 2);
}

TEST_CASE("BibleClient parses paragraph with verse content") {
    APIClient api;
    BibleClient client(api, 3034);
    std::string html = "<div class=\"p\">"
        "<span class=\"yv-v\" v=\"1\"></span>"
        "<span class=\"yv-vlbl\">1</span>"
        "In the beginning God created the heavens and the earth."
        "</div>";
    auto result = BibleClientTest::Parse(client, html, "GEN", 1);
    REQUIRE(result.has_value());
    REQUIRE(result->segments.size() >= 2);
    CHECK(result->segments[0].type == SegmentType::ParagraphBreak);
    CHECK(result->segments[1].type == SegmentType::VerseText);
    CHECK(result->segments[1].verseStart == 1);
    CHECK(result->segments[1].verseEnd == 1);
    CHECK_FALSE(result->words.empty());
    CHECK(result->words[0].verseId == 1);
}

TEST_CASE("BibleClient strips footnotes from HTML") {
    APIClient api;
    BibleClient client(api, 3034);
    std::string html = "<div class=\"p\">"
        "<span class=\"yv-v\" v=\"1\"></span>"
        "<span class=\"yv-vlbl\">1</span>"
        "Visible text"
        "<span class=\"yv-n f\"><span class=\"fr\">1:1 </span><span class=\"ft\">footnote</span></span>"
        " more text"
        "</div>";
    auto result = BibleClientTest::Parse(client, html, "GEN", 1);
    REQUIRE(result.has_value());
    bool hasFootnote = false;
    for (const auto& w : result->words) {
        if (w.text.find("footnote") != std::string::npos) hasFootnote = true;
    }
    CHECK_FALSE(hasFootnote);
}

TEST_CASE("BibleClient parses poetry lines q1 and q2") {
    APIClient api;
    BibleClient client(api, 3034);
    std::string html = "<div class=\"q1\">"
        "<span class=\"yv-v\" v=\"1\"></span>"
        "<span class=\"yv-vlbl\">1</span>"
        "Blessed is the man"
        "</div>"
        "<div class=\"q2\">"
        "who walks not in the counsel"
        "</div>";
    auto result = BibleClientTest::Parse(client, html, "PSA", 1);
    REQUIRE(result.has_value());
    REQUIRE(result->segments.size() == 2);
    CHECK(result->segments[0].type == SegmentType::PoetryLine);
    CHECK(result->segments[0].level == 1);
    CHECK(result->segments[1].type == SegmentType::PoetryLine);
    CHECK(result->segments[1].level == 2);
}

TEST_CASE("BibleClient decodes HTML entities") {
    APIClient api;
    BibleClient client(api, 3034);
    std::string html = "<div class=\"p\">"
        "<span class=\"yv-v\" v=\"1\"></span>"
        "<span class=\"yv-vlbl\">1</span>"
        "A &amp; B &lt; C &gt; D &quot;E&quot;"
        "</div>";
    auto result = BibleClientTest::Parse(client, html, "GEN", 1);
    REQUIRE(result.has_value());
    CHECK_FALSE(result->words.empty());
    bool hasAmpersand = false;
    for (const auto& w : result->words) {
        if (w.text == "&") hasAmpersand = true;
        if (w.text == "&amp;") hasAmpersand = false;
    }
    CHECK(hasAmpersand);
}

TEST_CASE("BibleClient handles empty HTML") {
    APIClient api;
    BibleClient client(api, 3034);
    auto result = BibleClientTest::Parse(client, "", "GEN", 1);
    REQUIRE(result.has_value());
    CHECK(result->words.empty());
    CHECK(result->segments.empty());
}

TEST_CASE("BibleClient parses multiple verses in a chapter") {
    APIClient api;
    BibleClient client(api, 3034);
    std::string html = "<div class=\"p\">"
        "<span class=\"yv-v\" v=\"1\"></span><span class=\"yv-vlbl\">1</span>Verse one"
        "<span class=\"yv-v\" v=\"2\"></span><span class=\"yv-vlbl\">2</span>Verse two"
        "<span class=\"yv-v\" v=\"3\"></span><span class=\"yv-vlbl\">3</span>Verse three"
        "</div>";
    auto result = BibleClientTest::Parse(client, html, "GEN", 1);
    REQUIRE(result.has_value());
    REQUIRE(result->segments.size() >= 2);
    int verseCount = 0;
    for (const auto& seg : result->segments) {
        if (seg.type == SegmentType::VerseText) verseCount++;
    }
    CHECK(verseCount == 3);
}

// ── CompositeProvider Tests ───────────────────────────────────────────

namespace {
    class FailingProvider : public ChapterProvider {
    public:
        bool HasChapter(const std::string&, int) const override { return false; }
        std::optional<ChapterData> LoadChapter(const std::string&, int) override {
            return std::nullopt;
        }
        const char* ProviderName() const override { return "FailingProvider"; }
    };

    class AlwaysGenesisProvider : public ChapterProvider {
    public:
        bool HasChapter(const std::string&, int) const override { return true; }
        std::optional<ChapterData> LoadChapter(const std::string& bookId, int chapter) override {
            ChapterData data;
            data.bookId = bookId;
            data.chapterNum = chapter;
            Word w;
            w.id = 0;
            w.verseId = 1;
            w.text = "Genesis";
            data.words.push_back(w);
            return data;
        }
        const char* ProviderName() const override { return "AlwaysGenesis"; }
    };
}

TEST_CASE("CompositeProvider returns primary result when it succeeds") {
    AlwaysGenesisProvider primary;
    FailingProvider fallback;
    CompositeProvider composite(primary, fallback);
    auto result = composite.LoadChapter("GEN", 1);
    REQUIRE(result.has_value());
    CHECK(result->bookId == "GEN");
    CHECK(result->chapterNum == 1);
    CHECK(result->words[0].text == "Genesis");
}

TEST_CASE("CompositeProvider falls back when primary fails") {
    FailingProvider primary;
    AlwaysGenesisProvider fallback;
    CompositeProvider composite(primary, fallback);
    auto result = composite.LoadChapter("GEN", 1);
    REQUIRE(result.has_value());
    CHECK(result->bookId == "GEN");
    CHECK(result->chapterNum == 1);
}

TEST_CASE("CompositeProvider returns nullopt when both fail") {
    FailingProvider primary;
    FailingProvider fallback;
    CompositeProvider composite(primary, fallback);
    auto result = composite.LoadChapter("GEN", 1);
    CHECK_FALSE(result.has_value());
}

TEST_CASE("CompositeProvider HasChapter returns true if either provider has it") {
    AlwaysGenesisProvider hasIt;
    FailingProvider doesNot;
    CompositeProvider composite(hasIt, doesNot);
    CHECK(composite.HasChapter("GEN", 1));
    CompositeProvider composite2(doesNot, hasIt);
    CHECK(composite2.HasChapter("GEN", 1));
}

TEST_CASE("CompositeProvider HasChapter returns false when neither has it") {
    FailingProvider a;
    FailingProvider b;
    CompositeProvider composite(a, b);
    CHECK_FALSE(composite.HasChapter("GEN", 999));
}

TEST_CASE("CompositeProvider ProviderName") {
    AlwaysGenesisProvider p;
    FailingProvider f;
    CompositeProvider composite(p, f);
    CHECK(std::string(composite.ProviderName()) == "CompositeProvider");
}

// ── Highlight System Tests ────────────────────────────────────────────

TEST_CASE("Highlighter startSelection records word ID") {
    InMemoryStorage store;
    Highlighter h(store);
    h.startSelection(42);
    h.endSelection();
    auto& highlights = h.getHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].startWord == 42);
    CHECK(highlights[0].endWord == 42);
}

TEST_CASE("Highlighter selection range covers start to end") {
    InMemoryStorage store;
    Highlighter h(store);
    h.startSelection(5);
    h.updateSelection(12);
    h.endSelection();
    auto& highlights = h.getHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].startWord == 5);
    CHECK(highlights[0].endWord == 12);
}

TEST_CASE("Highlighter handles reverse drag direction") {
    InMemoryStorage store;
    Highlighter h(store);
    h.startSelection(20);
    h.updateSelection(10);
    h.endSelection();
    auto& highlights = h.getHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].startWord == 10);
    CHECK(highlights[0].endWord == 20);
}

TEST_CASE("Highlighter does not create highlight without endSelection") {
    InMemoryStorage store;
    Highlighter h(store);
    h.startSelection(5);
    CHECK(h.getHighlights().empty());
}

TEST_CASE("Highlighter isWordHighlighted checks ranges") {
    InMemoryStorage store;
    Highlighter h(store);
    h.startSelection(5);
    h.updateSelection(10);
    h.endSelection();
    CHECK(h.isWordHighlighted(5));
    CHECK(h.isWordHighlighted(7));
    CHECK(h.isWordHighlighted(10));
    CHECK_FALSE(h.isWordHighlighted(4));
    CHECK_FALSE(h.isWordHighlighted(11));
}

TEST_CASE("Highlighter getHighlightForWord returns color for highlighted word") {
    InMemoryStorage store;
    Highlighter h(store);
    h.startSelection(5);
    h.updateSelection(10);
    h.endSelection();
    Color c = h.getHighlightForWord(7);
    CHECK(c.a > 0);
    Color c2 = h.getHighlightForWord(99);
    CHECK(c2.a == 0);
}

TEST_CASE("Highlighter loads persisted highlights on construction") {
    InMemoryStorage store;
    store.saveHighlight({1, 10, 20, 1});
    Highlighter h(store);
    CHECK(h.isWordHighlighted(15));
    CHECK_FALSE(h.isWordHighlighted(5));
}

TEST_CASE("InMemoryStorage save and load round-trip") {
    InMemoryStorage store;
    store.saveHighlight({1, 10, 20, 1});
    store.saveHighlight({2, 30, 40, 1});
    auto highlights = store.loadHighlights();
    REQUIRE(highlights.size() == 2);
    CHECK(highlights[0].startWord == 10);
    CHECK(highlights[1].startWord == 30);
}

TEST_CASE("InMemoryStorage remove highlight") {
    InMemoryStorage store;
    store.saveHighlight({1, 10, 20, 1});
    store.saveHighlight({2, 30, 40, 1});
    store.removeHighlight(1);
    auto highlights = store.loadHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].id == 2);
}

// ── PersistenceManager Tests ──────────────────────────────────────────

TEST_CASE("PersistenceManager save and load round-trip") {
    PersistenceManager pm(":memory:");
    pm.saveHighlight({1, 10, 20, 1});
    pm.saveHighlight({2, 30, 40, 1});
    auto highlights = pm.loadHighlights();
    REQUIRE(highlights.size() == 2);
    CHECK(highlights[0].startWord == 10);
    CHECK(highlights[0].endWord == 20);
    CHECK(highlights[1].startWord == 30);
    CHECK(highlights[1].endWord == 40);
}

TEST_CASE("PersistenceManager remove highlight") {
    PersistenceManager pm(":memory:");
    pm.saveHighlight({1, 10, 20, 1});
    pm.saveHighlight({2, 30, 40, 1});
    pm.removeHighlight(1);
    auto highlights = pm.loadHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].id == 2);
}

TEST_CASE("PersistenceManager highlight types") {
    PersistenceManager pm(":memory:");
    HighlightType t{1, "Pink", {255, 192, 203, 100}};
    pm.saveHighlightType(t);
    auto types = pm.loadHighlightTypes();
    REQUIRE(types.size() == 1);
    CHECK(types[0].id == 1);
    CHECK(types[0].name == "Pink");
    CHECK(types[0].color.r == 255);
    CHECK(types[0].color.g == 192);
    CHECK(types[0].color.b == 203);
}

TEST_CASE("PersistenceManager preferences") {
    PersistenceManager pm(":memory:");
    pm.setPreference("theme", "dark");
    pm.setPreference("font_size", "18");
    CHECK(pm.getPreference("theme", "") == "dark");
    CHECK(pm.getPreference("font_size", "") == "18");
    CHECK(pm.getPreference("nonexistent", "default") == "default");
}
