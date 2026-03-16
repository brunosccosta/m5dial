# M5Dial Smart Home UI - LVGL Pro Editor Project

This folder contains the UI definition files for the M5Dial Smart Home Controller, designed to be edited using **LVGL Pro Editor**.

## Folder Structure

```
ui/
├── project.xml          # Project configuration
├── globals.xml          # Global styles, colors, fonts, and constants
├── components/          # Reusable UI components
│   └── menu_item.xml    # Menu item component
├── screens/             # Screen definitions
│   ├── main_menu.xml    # Main navigation menu
│   ├── lamp_control.xml # Lamp control interface
│   └── climate_control.xml  # Climate control interface
└── widgets/             # Custom widgets (if needed)
```

## Workflow

### 1. Design UI in LVGL Pro Editor

1. **Download LVGL Pro Editor**: Get it from https://pro.lvgl.io
2. **Open this project**: File → Open Project → Select the `ui/` folder
3. **Edit screens and components**: Use the visual editor to modify the UI
4. **Preview in real-time**: See changes instantly in the editor

### 2. Export to C Code

Once you're satisfied with the UI design:

1. In LVGL Pro Editor: **File → Export → C Code**
2. Set export path to: `../src/ui_generated/`
3. Click **Export**

This generates:
- `ui_generated/m5dial_ui.h` - Main header file
- `ui_generated/m5dial_ui.c` - Initialization code
- `ui_generated/m5dial_ui_gen.h` - Generated header
- `ui_generated/m5dial_ui_gen.c` - Generated implementation
- `ui_generated/screens/*.c` and `*.h` - Screen files
- `ui_generated/components/*.c` and `*.h` - Component files

### 3. Build and Upload

The PlatformIO project is already configured to include the generated code:

```bash
# Build the firmware
pio run

# Upload to M5Dial
pio run --target upload

# Monitor serial output
pio run --target monitor
```

## XML File Format

### Screens (`screens/*.xml`)

Screens are top-level containers for complete UI views:

```xml
<screen>
    <view width="240" height="240">
        <!-- Your UI elements here -->
        <lv_label text="Hello World" align="center"/>
    </view>
</screen>
```

### Components (`components/*.xml`)

Components are reusable UI elements with custom APIs:

```xml
<component>
    <api>
        <prop name="my_text" type="string" default="Default"/>
    </api>
    
    <view extends="lv_button">
        <lv_label text="$my_text"/>
    </view>
</component>
```

### Global Definitions (`globals.xml`)

Shared styles, colors, fonts, and constants:

```xml
<globals>
    <consts>
        <color name="PRIMARY_COLOR" value="0x00A8E1"/>
    </consts>
    
    <styles>
        <style name="my_style">
            <bg_color>#PRIMARY_COLOR</bg_color>
        </style>
    </styles>
</globals>
```

## Integration with Application Code

The generated UI code is integrated in `src/main.cpp`:

```cpp
#include "ui_generated/m5dial_ui.h"

void setup() {
    M5Dial.begin();
    lv_init();
    
    // Initialize the UI project
    m5dial_ui_init("");  // Empty string if no external assets
    
    // Load the main menu screen
    lv_screen_load(main_menu_create());
}
```

## Connecting UI Events to Application Logic

After exporting, you can connect UI events to your application code by registering callbacks:

```cpp
// In your main.cpp or a separate file
void on_lamp_button_clicked(lv_event_t * e) {
    // Your Home Assistant control code here
    Serial.println("Lamp button clicked!");
}

void setup() {
    // ... LVGL init ...
    
    m5dial_ui_init("");
    
    // Get the button object and add event handler
    lv_obj_t * screen = main_menu_create();
    lv_obj_t * btn = lv_obj_get_child(screen, 0); // Or use ID lookup
    lv_obj_add_event_cb(btn, on_lamp_button_clicked, LV_EVENT_CLICKED, NULL);
    
    lv_screen_load(screen);
}
```

## Tips

1. **Round Display**: M5Dial has a 240x240 round display. Design with this in mind
2. **Fonts**: Built-in Montserrat fonts (14, 24, 32) are available
3. **Colors**: Use hex colors (e.g., `0xFF0000` for red)
4. **Layout**: Use flexbox for responsive layouts
5. **Memory**: Keep UI simple to save RAM on ESP32-S3

## Resources

- **LVGL Pro Editor Docs**: https://docs.lvgl.io/master/details/xml/
- **LVGL Documentation**: https://docs.lvgl.io/
- **M5Dial Hardware**: https://docs.m5stack.com/en/core/M5Dial
- **Project Overview**: See `../agents.md` for project requirements

## Current Status

The UI files provided are **starter templates**. Open them in LVGL Pro Editor to:
- Customize the layout and styling
- Add more screens for additional device types
- Create reusable components
- Add animations and transitions
- Test with different configurations

Happy designing! 🎨
