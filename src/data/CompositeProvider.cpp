#include "CompositeProvider.h"

namespace theword::data {

CompositeProvider::CompositeProvider(ChapterProvider& primary)
    : primary(&primary) {}

void CompositeProvider::SetPrimary(ChapterProvider& provider) {
    primary = &provider;
}

std::optional<ChapterData> CompositeProvider::LoadChapter(
        const std::string& bookId, int chapter) {
    return primary->LoadChapter(bookId, chapter);
}

const char* CompositeProvider::ProviderName() const {
    return primary->ProviderName();
}

} // namespace theword::data
