#ifndef COMPOSITE_PROVIDER_H
#define COMPOSITE_PROVIDER_H

#include "ChapterProvider.h"

class CompositeProvider : public ChapterProvider {
public:
    CompositeProvider(ChapterProvider& primary, ChapterProvider& fallback);

    void SetPrimary(ChapterProvider& provider);

    bool HasChapter(const std::string& bookId, int chapter) const override;
    std::optional<ChapterData> LoadChapter(
        const std::string& bookId, int chapter) override;
    const char* ProviderName() const override;

private:
    ChapterProvider* primary;
    ChapterProvider& fallback;
};

#endif
