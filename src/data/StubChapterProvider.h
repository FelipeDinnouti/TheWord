#ifndef STUB_CHAPTER_PROVIDER_H
#define STUB_CHAPTER_PROVIDER_H

#include "ChapterProvider.h"

class StubChapterProvider : public ChapterProvider {
public:
    bool HasChapter(const std::string& bookId, int chapter) const override;
    std::optional<ChapterData> LoadChapter(
        const std::string& bookId, int chapter) override;
    const char* ProviderName() const override;
};

#endif
