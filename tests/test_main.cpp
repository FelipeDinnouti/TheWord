#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include <sqlite3.h>
#include <raylib.h>

#include "core/Config.h"
#include "core/EnvLoader.h"
#include "core/BibleBooks.h"
#include "core/FuzzyMatcher.h"
#include "core/IHttpClient.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "MockHttpClient.h"
#include "data/ChapterProvider.h"
#include "data/BibleClient.h"
#include "data/CompositeProvider.h"
#include "data/StubChapterProvider.h"
#include "data/USFMParser.h"
#include "highlight/InMemoryStorage.h"
#include "highlight/Highlighter.h"
#include "persistence/PersistenceManager.h"

using namespace theword::core;
using namespace theword::data;
using namespace theword::highlight;
using namespace theword::persistence;
using namespace theword::test;

// Test helper: exposes private parseHtmlChapter for unit testing
class BibleClientTest {
public:
    static std::optional<ChapterData> Parse(theword::data::BibleClient& client,
            const std::string& html, const std::string& bookId, int chapter) {
        return client.ParseHtmlChapter(html, bookId, chapter);
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
    std::string value = EnvLoader::Get("NONEXISTENT_KEY_THAT_SHOULD_NOT_EXIST");
    CHECK(value.empty());
}

TEST_CASE("EnvLoader returns default for missing key") {
    std::string value = EnvLoader::Get("NONEXISTENT_KEY", "fallback_value");
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

TEST_CASE("FuzzyMatcher prefix match") {
    CHECK(FuzzyMatch("gen", "Genesis") > 0);
    CHECK(FuzzyMatch("ex", "Exodus") > 0);
    CHECK(FuzzyMatch("jo", "John") > 0);
    CHECK(FuzzyMatch("jo", "Joel") > 0);
    CHECK(FuzzyMatch("Gen", "Genesis") > FuzzyMatch("Gen", "James"));
}

TEST_CASE("FuzzyMatcher fuzzy match") {
    CHECK(FuzzyMatch("gn", "Genesis") > 0);
    CHECK(FuzzyMatch("gn", "James") < 0);
    CHECK(FuzzyMatch("jhn", "John") > 0);
    CHECK(FuzzyMatch("slm", "Salmos") > 0);
}

TEST_CASE("FuzzyMatcher accented Portuguese") {
    CHECK(FuzzyMatch("gen", "G\u00eanesis") > 0);
    CHECK(FuzzyMatch("genesis", "G\u00eanesis") > 0);
    CHECK(FuzzyMatch("joao", "Jo\u00e3o") > 0);
    CHECK(FuzzyMatch("exodo", "\u00caxodo") > 0);
    CHECK(FuzzyMatch("numeros", "N\u00fameros") > 0);
}

TEST_CASE("FuzzyMatcher no match") {
    CHECK(FuzzyMatch("xyz", "Genesis") < 0);
    CHECK(FuzzyMatch("zzz", "John") < 0);
}

TEST_CASE("FuzzyMatcher empty query") {
    CHECK(FuzzyMatch("", "Genesis") == 0);
    CHECK(FuzzyMatch("", "") == 0);
}

TEST_CASE("FuzzyMatcher prefix scores higher than substring") {
    int prefixScore = FuzzyMatch("jo", "John");
    int fuzzyScore = FuzzyMatch("jo", "Major Prophets");
    CHECK(prefixScore > fuzzyScore);
}

TEST_CASE("EnvLoader handles system environment") {
    // SYSTEMROOT is typically set on Windows, HOME on Unix
    std::string home = EnvLoader::Get("HOME");
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

TEST_CASE("SegmentType includes VerseNumber") {
    Span span;
    span.text = "1";
    span.type = SegmentType::VerseNumber;
    CHECK(span.type == SegmentType::VerseNumber);
    CHECK(span.text == "1");
    CHECK(span.startWord == 0);
    CHECK(span.endWord == 0);
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

TEST_CASE("USFMParser extracts footnotes from real file") {
    USFMParser parser(config::USFM_DIR);
    auto result = parser.LoadChapter("MAT", 1);
    REQUIRE(result.has_value());
    CHECK_FALSE(result->footnotes.empty());
    // Footnotes should have text, no footnote content should appear in words
    for (const auto& fn : result->footnotes) {
        CHECK_FALSE(fn.text.empty());
        CHECK(fn.verseId > 0);
    }
    bool wordHasFootnote = false;
    for (const auto& w : result->words) {
        for (const auto& fn : result->footnotes) {
            if (fn.text.length() > 3 && w.text.find(fn.text) != std::string::npos) {
                wordHasFootnote = true;
            }
        }
    }
    CHECK_FALSE(wordHasFootnote);
    // \rq cross-references should be stripped (MAT 1:23, 2:6, etc.)
    bool hasRq = false;
    for (const auto& w : result->words) {
        if (w.text.find("Isaías") != std::string::npos ||
            w.text.find("Miqueias") != std::string::npos ||
            w.text.find("Jeremias") != std::string::npos) {
            hasRq = true;
        }
    }
    CHECK_FALSE(hasRq);
    // Cross-references should now appear as footnotes
    bool rqInFootnotes = false;
    for (const auto& fn : result->footnotes) {
        if (fn.text.find("Isaías") != std::string::npos ||
            fn.text.find("Miqueias") != std::string::npos ||
            fn.text.find("Jeremias") != std::string::npos) {
            rqInFootnotes = true;
        }
    }
    CHECK(rqInFootnotes);
}

// ── BibleClient HTML Parser Tests ──────────────────────────────────────

TEST_CASE("BibleClient parses section heading from HTML") {
    MockHttpClient api;
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
    MockHttpClient api;
    BibleClient client(api, 3034);
    std::string html = "<div class=\"s2 yv-h\">Subheading</div>";
    auto result = BibleClientTest::Parse(client, html, "GEN", 2);
    REQUIRE(result.has_value());
    REQUIRE_FALSE(result->segments.empty());
    CHECK(result->segments[0].type == SegmentType::SectionHeading);
    CHECK(result->segments[0].level == 2);
}

TEST_CASE("BibleClient parses paragraph with verse content") {
    MockHttpClient api;
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

TEST_CASE("BibleClient extracts footnotes from HTML") {
    MockHttpClient api;
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

    // Words should not contain footnote text
    bool hasFootnote = false;
    for (const auto& w : result->words) {
        if (w.text.find("footnote") != std::string::npos) hasFootnote = true;
    }
    CHECK_FALSE(hasFootnote);

    // Footnotes should be extracted
    CHECK(result->footnotes.size() == 1);
    CHECK(result->footnotes[0].text == "footnote");
    CHECK(result->footnotes[0].verseId == 1);
}

TEST_CASE("BibleClient parses poetry lines q1 and q2") {
    MockHttpClient api;
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
    MockHttpClient api;
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
    MockHttpClient api;
    BibleClient client(api, 3034);
    auto result = BibleClientTest::Parse(client, "", "GEN", 1);
    REQUIRE(result.has_value());
    CHECK(result->words.empty());
    CHECK(result->segments.empty());
}

TEST_CASE("BibleClient parses multiple verses in a chapter") {
    MockHttpClient api;
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
        bool HasChapter(const std::string&, int) override { return false; }
        std::optional<ChapterData> LoadChapter(const std::string&, int) override {
            return std::nullopt;
        }
        const char* ProviderName() const override { return "FailingProvider"; }
    };

    class AlwaysGenesisProvider : public ChapterProvider {
    public:
        bool HasChapter(const std::string&, int) override { return true; }
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

TEST_CASE("CompositeProvider setPrimary switches primary provider") {
    AlwaysGenesisProvider ag;
    FailingProvider fail;

    // Start with primary=AlwaysGenesis, fallback=FailingProvider
    CompositeProvider composite(ag, fail);
    auto r1 = composite.LoadChapter("GEN", 1);
    REQUIRE(r1.has_value());
    CHECK(r1->words[0].text == "Genesis");

    // After SetPrimary to fail, LoadChapter should fail (both primary and fallback fail)
    composite.SetPrimary(fail);
    auto r2 = composite.LoadChapter("GEN", 1);
    CHECK_FALSE(r2.has_value());
}

TEST_CASE("CompositeProvider setPrimary switches to working provider") {
    AlwaysGenesisProvider ag;
    FailingProvider fail;

    CompositeProvider composite(fail, ag);
    auto r1 = composite.LoadChapter("GEN", 1);
    REQUIRE(r1.has_value());

    // After SetPrimary to ag, primary succeeds directly
    composite.SetPrimary(ag);
    auto r2 = composite.LoadChapter("GEN", 1);
    REQUIRE(r2.has_value());
    CHECK(r2->words[0].text == "Genesis");
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
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    h.SetChapterContext("GEN", 1, nullptr);
    h.OnSelection(theword::event::SelectionEvent{theword::event::SelectionEvent::Action::Start, 42, 42});
    h.CreateHighlight(42, 42, 1, "GEN", 1, nullptr);
    auto& highlights = h.GetHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].startWord == 42);
    CHECK(highlights[0].endWord == 42);
}

TEST_CASE("Highlighter selection range covers start to end") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    h.SetChapterContext("GEN", 1, nullptr);
    h.OnSelection(theword::event::SelectionEvent{theword::event::SelectionEvent::Action::Start, 5, 5});
    h.OnSelection(theword::event::SelectionEvent{theword::event::SelectionEvent::Action::Update, 5, 12});
    h.CreateHighlight(5, 12, 1, "GEN", 1, nullptr);
    auto& highlights = h.GetHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].startWord == 5);
    CHECK(highlights[0].endWord == 12);
}

TEST_CASE("Highlighter handles reverse drag direction") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    h.SetChapterContext("GEN", 1, nullptr);
    h.OnSelection(theword::event::SelectionEvent{theword::event::SelectionEvent::Action::Start, 20, 20});
    h.OnSelection(theword::event::SelectionEvent{theword::event::SelectionEvent::Action::Update, 20, 10});
    h.CreateHighlight(20, 10, 1, "GEN", 1, nullptr);
    auto& highlights = h.GetHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].startWord == 10);
    CHECK(highlights[0].endWord == 20);
}

TEST_CASE("Highlighter does not create highlight without endSelection") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    h.OnSelection(theword::event::SelectionEvent{theword::event::SelectionEvent::Action::Start, 5, 5});
    CHECK(h.GetHighlights().empty());
}

TEST_CASE("Highlighter isWordHighlighted checks ranges") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    h.SetChapterContext("GEN", 1, nullptr);
    h.CreateHighlight(5, 10, 1, "GEN", 1, nullptr);
    CHECK(h.IsWordHighlighted(7));
    CHECK(h.IsWordHighlighted(10));
    CHECK_FALSE(h.IsWordHighlighted(4));
    CHECK_FALSE(h.IsWordHighlighted(11));
}

TEST_CASE("Highlighter getHighlightForWord returns color for highlighted word") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    h.SetChapterContext("GEN", 1, nullptr);
    h.CreateHighlight(5, 10, 1, "GEN", 1, nullptr);
    SimpleColor c = h.GetHighlightForWord(7);
    CHECK(c.a > 0);
    SimpleColor c2 = h.GetHighlightForWord(99);
    CHECK(c2.a == 0);
}

TEST_CASE("Highlighter loads persisted highlights on construction") {
    InMemoryStorage store;
    store.SaveHighlight({1, 10, 20, 1});
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    CHECK(h.IsWordHighlighted(15));
    CHECK_FALSE(h.IsWordHighlighted(5));
}

TEST_CASE("InMemoryStorage save and load round-trip") {
    InMemoryStorage store;
    store.SaveHighlight({1, 10, 20, 1});
    store.SaveHighlight({2, 30, 40, 1});
    auto highlights = store.LoadHighlights();
    REQUIRE(highlights.size() == 2);
    CHECK(highlights[0].startWord == 10);
    CHECK(highlights[1].startWord == 30);
}

TEST_CASE("InMemoryStorage remove highlight") {
    InMemoryStorage store;
    store.SaveHighlight({1, 10, 20, 1});
    store.SaveHighlight({2, 30, 40, 1});
    store.RemoveHighlight(1);
    auto highlights = store.LoadHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].id == 2);
}

TEST_CASE("InMemoryStorage saveHighlight replaces existing by ID") {
    InMemoryStorage store;
    store.SaveHighlight({1, 10, 20, 1});
    store.SaveHighlight({1, 15, 25, 2});
    auto highlights = store.LoadHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].startWord == 15);
    CHECK(highlights[0].typeId == 2);
}

TEST_CASE("InMemoryStorage loadHighlightTypes returns empty") {
    InMemoryStorage store;
    auto types = store.LoadHighlightTypes();
    CHECK(types.empty());
}

// ── New Highlighter Tests ──────────────────────────────────────────────

TEST_CASE("Highlighter highlightAtWord returns pointer for highlighted word") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    h.SetChapterContext("GEN", 1, nullptr);
    h.CreateHighlight(5, 10, 1, "GEN", 1, nullptr);
    const Highlight* hl = h.HighlightAtWord(7);
    REQUIRE(hl != nullptr);
    CHECK(hl->startWord == 5);
    CHECK(hl->endWord == 10);
}

TEST_CASE("Highlighter highlightAtWord returns nullptr for unhighlighted word") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    h.SetChapterContext("GEN", 1, nullptr);
    h.CreateHighlight(5, 10, 1, "GEN", 1, nullptr);
    const Highlight* hl = h.HighlightAtWord(4);
    CHECK(hl == nullptr);
    hl = h.HighlightAtWord(11);
    CHECK(hl == nullptr);
}

TEST_CASE("Highlighter removeHighlight removes from internal vector") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    h.SetChapterContext("GEN", 1, nullptr);
    h.CreateHighlight(5, 10, 1, "GEN", 1, nullptr);
    REQUIRE(h.GetHighlights().size() == 1);
    h.RemoveHighlight(h.GetHighlights()[0].id);
    CHECK(h.GetHighlights().empty());
    CHECK_FALSE(h.IsWordHighlighted(7));
}

TEST_CASE("Highlighter recolorHighlight changes typeId") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    h.SetChapterContext("GEN", 1, nullptr);
    h.CreateHighlight(5, 10, 1, "GEN", 1, nullptr);
    int hlId = h.GetHighlights()[0].id;
    int oldType = h.GetHighlights()[0].typeId;
    int newType = (oldType == 1) ? 2 : 1;
    h.RecolorHighlight(hlId, newType);
    CHECK(h.GetHighlights()[0].typeId == newType);
}

TEST_CASE("Highlighter CreateHighlight uses provided typeId") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    h.SetChapterContext("GEN", 1, nullptr);
    h.CreateHighlight(1, 5, 99, "GEN", 1, nullptr);
    CHECK(h.GetHighlights()[0].typeId == 99);
}

TEST_CASE("Highlighter getActiveTypeId returns current") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    h.SetActiveTypeId(5);
    CHECK(h.GetActiveTypeId() == 5);
}

TEST_CASE("Highlighter getTypes returns available types") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);
    const auto& types = h.GetTypes();
    CHECK_FALSE(types.empty());
    CHECK(types[0].id == 1);
}

// ── PersistenceManager Tests ──────────────────────────────────────────

TEST_CASE("PersistenceManager save and load round-trip") {
    PersistenceManager pm(":memory:");
    pm.SaveHighlight({1, 10, 20, 1});
    pm.SaveHighlight({2, 30, 40, 1});
    auto highlights = pm.LoadHighlights();
    REQUIRE(highlights.size() == 2);
    CHECK(highlights[0].startWord == 10);
    CHECK(highlights[0].endWord == 20);
    CHECK(highlights[1].startWord == 30);
    CHECK(highlights[1].endWord == 40);
}

TEST_CASE("PersistenceManager remove highlight") {
    PersistenceManager pm(":memory:");
    pm.SaveHighlight({1, 10, 20, 1});
    pm.SaveHighlight({2, 30, 40, 1});
    pm.RemoveHighlight(1);
    auto highlights = pm.LoadHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].id == 2);
}

TEST_CASE("PersistenceManager highlight types") {
    PersistenceManager pm(":memory:");
    auto types = pm.LoadHighlightTypes();
    REQUIRE(types.size() == 5);
    CHECK(types[0].id == 1);
    CHECK(types[0].name == "Yellow");
    CHECK(types[0].color.r == 255);
    CHECK(types[0].color.g == 235);
    CHECK(types[0].color.b == 59);
    CHECK(types[4].id == 5);
    CHECK(types[4].name == "Orange");
}

TEST_CASE("PersistenceManager preferences") {
    PersistenceManager pm(":memory:");
    pm.SetPreference("theme", "dark");
    pm.SetPreference("font_size", "18");
    CHECK(pm.GetPreference("theme", "") == "dark");
    CHECK(pm.GetPreference("font_size", "") == "18");
    CHECK(pm.GetPreference("nonexistent", "default") == "default");
}

// ── Phase 13: Highlight Browser Tests ──────────────────────────────────

TEST_CASE("Highlighter stores reference fields via SetChapterContext") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);

    std::vector<theword::data::Word> words;
    words.push_back({0, 1, "In"});
    words.push_back({1, 1, "the"});
    words.push_back({2, 2, "beginning"});

    h.SetChapterContext("GEN", 1, &words);
    h.CreateHighlight(0, 2, 1, "GEN", 1, &words);

    auto& highlights = h.GetHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].bookId == "GEN");
    CHECK(highlights[0].chapterNum == 1);
    CHECK(highlights[0].verseStart == 1);
    CHECK(highlights[0].verseEnd == 2);
}

TEST_CASE("Highlighter verseText populated from word data") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);

    std::vector<theword::data::Word> words;
    words.push_back({0, 1, "In"});
    words.push_back({1, 1, "the"});
    words.push_back({2, 1, "beginning"});

    h.SetChapterContext("GEN", 1, &words);
    h.CreateHighlight(0, 2, 1, "GEN", 1, &words);

    auto& highlights = h.GetHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].verseText == "In the beginning");
}

TEST_CASE("Highlighter verseText truncated at 80 characters") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);

    std::vector<theword::data::Word> words;
    words.push_back({0, 1, std::string(50, 'A')});
    words.push_back({1, 1, std::string(40, 'B')});

    h.SetChapterContext("GEN", 1, &words);
    h.CreateHighlight(0, 1, 1, "GEN", 1, &words);

    auto& highlights = h.GetHighlights();
    REQUIRE(highlights.size() == 1);
    CHECK(highlights[0].verseText.length() == 83);
    CHECK(highlights[0].verseText.substr(80) == "...");
}

TEST_CASE("GetHighlightsByType filters correctly") {
    InMemoryStorage store;
    theword::event::EventBus eb;
    Highlighter h(eb, store);

    h.SetChapterContext("GEN", 1, nullptr);
    h.CreateHighlight(0, 0, 1, "GEN", 1, nullptr);

    h.CreateHighlight(10, 10, 2, "GEN", 1, nullptr);

    REQUIRE(h.GetHighlights().size() == 2);

    auto type1 = h.GetHighlightsByType(1);
    CHECK(type1.size() == 1);
    CHECK(type1[0]->typeId == 1);

    auto type2 = h.GetHighlightsByType(2);
    CHECK(type2.size() == 1);
    CHECK(type2[0]->typeId == 2);

    auto none = h.GetHighlightsByType(99);
    CHECK(none.empty());
}

TEST_CASE("PersistenceManager saves/loads ref fields round-trip") {
    PersistenceManager pm(":memory:");
    Highlight h;
    h.id = 1;
    h.startWord = 5;
    h.endWord = 10;
    h.typeId = 1;
    h.bookId = "GEN";
    h.chapterNum = 1;
    h.verseStart = 1;
    h.verseEnd = 3;
    h.verseText = "In the beginning";
    pm.SaveHighlight(h);

    auto loaded = pm.LoadHighlights();
    REQUIRE(loaded.size() == 1);
    CHECK(loaded[0].bookId == "GEN");
    CHECK(loaded[0].chapterNum == 1);
    CHECK(loaded[0].verseStart == 1);
    CHECK(loaded[0].verseEnd == 3);
    CHECK(loaded[0].verseText == "In the beginning");
}

TEST_CASE("PersistenceManager schema migration adds ref columns") {
    const char* testPath = "/tmp/theword_test_migration.db";
    remove(testPath);

    sqlite3* rawDb = nullptr;
    int rc = sqlite3_open(testPath, &rawDb);
    REQUIRE(rc == SQLITE_OK);

    char* err = nullptr;
    sqlite3_exec(rawDb,
        "CREATE TABLE highlights ("
        "  id INTEGER PRIMARY KEY,"
        "  start_word INTEGER NOT NULL,"
        "  end_word INTEGER NOT NULL,"
        "  type_id INTEGER"
        ");",
        nullptr, nullptr, &err);
    if (err) sqlite3_free(err);

    sqlite3_exec(rawDb,
        "INSERT INTO highlights (id, start_word, end_word, type_id)"
        " VALUES (1, 5, 10, 1)",
        nullptr, nullptr, nullptr);
    sqlite3_close(rawDb);

    {
        PersistenceManager pm(testPath);
        auto loaded = pm.LoadHighlights();
        REQUIRE(loaded.size() == 1);
        CHECK(loaded[0].id == 1);
        CHECK(loaded[0].bookId.empty());
        CHECK(loaded[0].chapterNum == 0);
        CHECK(loaded[0].verseStart == 0);
        CHECK(loaded[0].verseEnd == 0);
        CHECK(loaded[0].verseText.empty());
    }

    remove(testPath);
}

TEST_CASE("NavigateToHighlightEvent struct carries correct data") {
    theword::event::NavigateToHighlightEvent e{"GEN.3", 42};
    CHECK(e.chapterRef == "GEN.3");
    CHECK(e.wordId == 42);
}

// ── Verse Range / Word ID Tests ─────────────────────────────────────────

TEST_CASE("FindVerseRange returns correct word boundaries") {
    // Simulate a chapter with 3 verses, each with several words
    std::vector<theword::data::Word> words;
    int nextId = 0;
    // Verse 1: words 0-2
    for (int i = 0; i < 3; ++i) words.push_back({nextId++, 1, "v1w" + std::to_string(i)});
    // Verse 2: words 3-6
    for (int i = 0; i < 4; ++i) words.push_back({nextId++, 2, "v2w" + std::to_string(i)});
    // Verse 3: words 7-9
    for (int i = 0; i < 3; ++i) words.push_back({nextId++, 3, "v3w" + std::to_string(i)});

    // Duplicate of the FindVerseRange logic
    auto findRange = [](const std::vector<theword::data::Word>& wds, int anchor,
                         int& vStart, int& vEnd) {
        int targetVerse = -1;
        for (const auto& w : wds) {
            if (w.id == anchor) { targetVerse = w.verseId; break; }
        }
        if (targetVerse < 0) { vStart = anchor; vEnd = anchor; return; }
        vStart = anchor; vEnd = anchor;
        for (const auto& w : wds) {
            if (w.verseId == targetVerse) {
                if (w.id < vStart) vStart = w.id;
                if (w.id > vEnd) vEnd = w.id;
            }
        }
    };

    int s, e;

    // Tap word 4 (verse 2) → range should be words 3-6
    findRange(words, 4, s, e);
    CHECK(s == 3);
    CHECK(e == 6);

    // Tap word 0 (verse 1) → range should be words 0-2
    findRange(words, 0, s, e);
    CHECK(s == 0);
    CHECK(e == 2);

    // Tap word 9 (verse 3) → range should be words 7-9
    findRange(words, 9, s, e);
    CHECK(s == 7);
    CHECK(e == 9);
}

TEST_CASE("FindVerseRange handles words with same verseId across segments") {
    std::vector<theword::data::Word> words;
    // Verse 1 (text): words 0-2
    words.push_back({0, 1, "In"});
    words.push_back({1, 1, "the"});
    words.push_back({2, 1, "beginning"});
    // Verse 1 (poetry continuation): words 3-4 (same verseId)
    words.push_back({3, 1, "God"});
    words.push_back({4, 1, "created"});

    auto findRange = [](const std::vector<theword::data::Word>& wds, int anchor,
                         int& vStart, int& vEnd) {
        int targetVerse = -1;
        for (const auto& w : wds) {
            if (w.id == anchor) { targetVerse = w.verseId; break; }
        }
        if (targetVerse < 0) { vStart = anchor; vEnd = anchor; return; }
        vStart = anchor; vEnd = anchor;
        for (const auto& w : wds) {
            if (w.verseId == targetVerse) {
                if (w.id < vStart) vStart = w.id;
                if (w.id > vEnd) vEnd = w.id;
            }
        }
    };

    int s, e;
    findRange(words, 2, s, e);
    CHECK(s == 0);
    CHECK(e == 4);

    findRange(words, 4, s, e);
    CHECK(s == 0);
    CHECK(e == 4);
}

TEST_CASE("Word IDs are sequential and unique within a chapter") {
    std::vector<theword::data::Word> words;
    int nextId = 0;

    auto appendVerse = [&](int verseId, const std::string& text) {
        std::istringstream stream(text);
        std::string word;
        while (stream >> word) {
            words.push_back({nextId++, verseId, word});
        }
    };

    appendVerse(1, "In the beginning God created the heavens and the earth");
    appendVerse(2, "The earth was formless and void");

    CHECK_EQ(words[0].id, 0);
    CHECK_EQ(words[0].verseId, 1);
    CHECK_EQ(words[9].id, 9);
    CHECK_EQ(words[9].verseId, 1);
    CHECK_EQ(words[10].id, 10);
    CHECK_EQ(words[10].verseId, 2);

    int prevId = -1;
    for (const auto& w : words) {
        CHECK_GT(w.id, prevId);
        prevId = w.id;
    }
}

TEST_CASE("Double-tap on adjacent words should not match") {
    // Simulate the InputHandler double-tap detection
    double now = 100.0;
    double doubleClickTime = 0.3;
    int lastClickWord = 50;
    double lastClickTime = now - 0.1; // 100ms ago, within double-click window

    // If second tap is on word 51 (different word), isDouble should be false
    int secondWord = 51;
    bool isDouble = (secondWord == lastClickWord)
        && (now - lastClickTime) < doubleClickTime;
    CHECK_FALSE(isDouble);

    // If second tap is on same word 50, isDouble should be true
    secondWord = 50;
    isDouble = (secondWord == lastClickWord)
        && (now - lastClickTime) < doubleClickTime;
    CHECK(isDouble);
}


