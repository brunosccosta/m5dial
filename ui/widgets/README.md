# Widgets

Create folders here for each custom widget with an XML file containing a `<widget>` tag.

Widgets are custom C-based LVGL widgets that extend LVGL's built-in widget set. Unlike components (which are XML-only), widgets require custom C code.

## When to use widgets

- Need custom rendering or drawing logic
- Require complex internal state management
- Performance-critical custom controls
- Integration with hardware-specific features

## When to use components instead

- Combining existing LVGL widgets
- Styling existing widgets differently
- Creating reusable UI patterns
- Most UI design needs

Most use cases should use **components** rather than widgets.
