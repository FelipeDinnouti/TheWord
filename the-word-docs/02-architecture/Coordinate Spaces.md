# Coordinate Spaces

> Status: Stable | Last Updated: 2026-06-21

The system operates in two coordinate spaces. Understanding the distinction is essential for rendering, input handling, and scroll management.

## Document Space

- Origin `(0, 0)` = top-left corner of Genesis 1:1
- All chapter positions and the scroll position are stored in document space
- Y increases downward through the entire Bible
- Document space is independent of window size

## Screen Space

- Origin `(0, 0)` = top-left corner of the drawable area (below the title bar)
- Input events (mouse clicks, touches) arrive in screen space
- Drawing operations use screen space

## Conversion

```
Document → Screen:  screenY = documentY - scrollY + contentTop
Screen → Document:  documentY = screenY + scrollY - contentTop
```

Where:
- `scrollY` = current scroll position in document space
- `contentTop` = Y offset of the content area (e.g., 60px for title bar)

## Anchor-Fixed Prepend

When content is prepended above the viewport (user scrolls past the top of the first loaded chapter):

1. Get the height of the prepended chapter: `prependHeight = chapter.totalHeight`
2. Insert the chapter at the beginning of the chapter list
3. Adjust scroll position: `scrollY += prependHeight`

This ensures the visible content before the prepend stays at the same screen position after.

## Resize Handling

When the window width changes:
1. Invalidate all cached layouts (max width changed)
2. Re-layout all loaded chapters
3. Adjust scroll position to keep the anchor verse at the same screen position
