#include "CompositeProvider.h"

namespace theword::data {

CompositeProvider::CompositeProvider(ChapterProvider& primary, ChapterProvider& fallback)
    : primary(&primary), fallback(fallback) {}

void CompositeProvider::SetPrimary(ChapterProvider& provider) {
    primary = &provider;
}

bool CompositeProvider::HasChapter(const std::string& bookId, int chapter) {
    return primary->HasChapter(bookId, chapter) || fallback.HasChapter(bookId, chapter);
}

std::optional<ChapterData> CompositeProvider::LoadChapter(
        const std::string& bookId, int chapter) {
    auto result = primary->LoadChapter(bookId, chapter);
    if (result) return result;
    return fallback.LoadChapter(bookId, chapter);
}

const char* CompositeProvider::ProviderName() const {
    return "CompositeProvider";
}

} // namespace theword::data
