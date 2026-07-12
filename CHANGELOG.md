# Changelog

## [1.6.2-alpha] — 2026-07-12

### Added
- Sector-based radial menu buttons — each button now has an exact pie-slice tap zone, eliminating dead zones between buttons
- Android lifecycle resilience — survives tab-out (window-gap), restores scroll position on resume, resets input state after surface loss
- API key validation — clear error message if BSB API key is missing

### Fixed
- Radial menu buttons over empty space (between lines, below last verse) now register taps correctly instead of dismissing the menu
- Multi-verse selection no longer collapses to one verse when releasing between lines
- Highlight recolor no longer creates duplicate highlights — overlap detection uses the full word range, not just the start word
- Êxodo (Exodus) misspelling corrected
- Scroll position saved across Android orientation changes

### Changed
- Networking migrated to mbedTLS + HTTPS for Android Bible API access
