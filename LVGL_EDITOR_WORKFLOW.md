# LVGL Pro Editor Workflow for M5Dial Smart Home

This document explains how to use LVGL Pro Editor to design the UI for your M5Dial Smart Home Controller.

## Table of Contents

1. [Overview](#overview)
2. [Getting Started](#getting-started)
3. [Development Workflow](#development-workflow)
4. [UI File Structure](#ui-file-structure)
5. [Integration with Application Code](#integration-with-application-code)
6. [Best Practices](#best-practices)
7. [Troubleshooting](#troubleshooting)

## Overview

The project now supports two UI modes:

### Mode 1: Manual UI (Current Default)
- Hand-coded `CarouselMenu` class
- Quick prototyping and testing
- Useful for initial development

### Mode 2: Generated UI (Recommended for Production)
- Visual design in LVGL Pro Editor
- XML-based UI definition
- Exports to C code
- Professional workflow with visual preview
- Easier to maintain and iterate

## Getting Started

### Prerequisites

1. **LVGL Pro Editor**: Download from [https://pro.lvgl.io](https://pro.lvgl.io)
   - Available for Windows, macOS, and Linux
   - Free tier available for evaluation

2. **PlatformIO**: Already configured in this project

### Installation

1. Download and install LVGL Pro Editor
2. No additional configuration needed - the project is ready!

## Development Workflow

### Step 1: Design UI in LVGL Pro Editor

1. **Open the Project**
   ```
   LVGL Pro Editor → File → Open Project
   Navigate to: M5Dial_SmartHome/ui/
   ```

2. **Edit Screens**
   - Open `screens/main_menu.xml` to edit the main menu
   - Open `screens/lamp_control.xml` for lamp controls
   - Open `screens/climate_control.xml` for climate controls

3. **Edit Components**
   - Open `components/menu_item.xml` for reusable menu items
   - Create new components as needed

4. **Preview in Real-Time**
   - The editor shows a live preview
   - See changes instantly as you edit XML
   - Test different configurations

### Step 2: Export to C Code

Once you're happy with the design:

1. **Export from Editor**
   ```
   File → Export → C Code
   ```

2. **Configure Export Settings**
   - Output path: `../src/ui_generated/`
   - This is already configured in `ui/project.xml`

3. **Click Export**
   - Generates `.c` and `.h` files
   - Creates `*_gen.c` and `*_gen.h` (auto-generated, don't edit)
   - Creates `*.c` and `*.h` (user files, safe to edit)

### Step 3: Switch to Generated UI Mode

1. **Open `src/main.cpp`**

2. **Change the mode**:
   ```cpp
   // Comment out:
   // #define USE_MANUAL_UI
   
   // Uncomment:
   #define USE_GENERATED_UI
   ```

3. **Build and Upload**:
   ```bash
   pio run --target upload
   ```

### Step 4: Connect Events to Application Logic

After exporting, connect UI events to your Home Assistant control code:

```cpp
// Example: In main.cpp or a separate HomeAssistant.cpp file

void on_lamp_button_clicked(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    
    // Your Home Assistant API call here
    Serial.println("Toggling lamp...");
    
    // Update UI
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    lv_label_set_text(label, "OFF");
}

void setup() {
    // ... LVGL initialization ...
    
    m5dial_ui_init("");
    lv_obj_t * screen = main_menu_create();
    
    // Find and connect event handlers
    // (You'll need to use object IDs or query objects)
    
    lv_screen_load(screen);
}
```

## UI File Structure

```
ui/
├── project.xml              # Project configuration
├── globals.xml              # Global styles, colors, fonts
├── screens/                 # Screen definitions
│   ├── main_menu.xml        # Main navigation
│   ├── lamp_control.xml     # Lamp controls
│   └── climate_control.xml  # Climate controls
├── components/              # Reusable components
│   └── menu_item.xml        # Menu item component
└── widgets/                 # Custom widgets (advanced)
```

### File Types

#### `project.xml`
Project-level configuration:
- Project name
- Export settings
- Build configurations

#### `globals.xml`
Shared resources:
- Color definitions
- Font configurations
- Global styles
- Constants

#### `screens/*.xml`
Complete UI views:
- Top-level screens
- Navigation endpoints
- Full layouts

#### `components/*.xml`
Reusable UI elements:
- Buttons with custom styling
- Menu items
- Cards
- List items

## Integration with Application Code

### Generated Code Structure

After export, you'll have:

```
src/ui_generated/
├── m5dial_ui.h              # Main header (user file - edit this)
├── m5dial_ui.c              # Main implementation (user file)
├── m5dial_ui_gen.h          # Generated header (DON'T EDIT)
├── m5dial_ui_gen.c          # Generated implementation (DON'T EDIT)
├── screens/
│   ├── main_menu_gen.c      # Generated (DON'T EDIT)
│   ├── main_menu_gen.h
│   ├── lamp_control_gen.c
│   └── ...
└── components/
    ├── menu_item_gen.c
    └── ...
```

### Initialization

```cpp
#include "ui_generated/m5dial_ui.h"

void setup() {
    M5Dial.begin();
    lv_init();
    // ... display driver setup ...
    
    // Initialize the UI (pass empty string if no external assets)
    m5dial_ui_init("");
    
    // Load a screen
    lv_obj_t * screen = main_menu_create();
    lv_screen_load(screen);
}
```

### Screen Navigation

```cpp
// Load different screens
void navigate_to_lamp_control() {
    lv_obj_t * lamp_screen = lamp_control_create();
    lv_screen_load_anim(lamp_screen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
}

void navigate_to_climate() {
    lv_obj_t * climate_screen = climate_control_create();
    lv_screen_load_anim(climate_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}
```

### Accessing UI Elements

Use object IDs (set in XML):

```xml
<lv_label id="lbl_status" text="Ready"/>
```

Then in code:

```cpp
// Get object by name
lv_obj_t * screen = main_menu_create();
lv_obj_t * status_label = lv_obj_get_child_by_name(screen, "lbl_status");
lv_label_set_text(status_label, "Updating...");
```

### Event Handling

Add event callbacks:

```cpp
void on_button_click(lv_event_t * e) {
    Serial.println("Button clicked!");
}

void setup() {
    // ... init ...
    lv_obj_t * screen = lamp_control_create();
    
    // Get button by name
    lv_obj_t * btn = lv_obj_get_child_by_name(screen, "btn_power");
    
    // Add event handler
    lv_obj_add_event_cb(btn, on_button_click, LV_EVENT_CLICKED, NULL);
    
    lv_screen_load(screen);
}
```

## Best Practices

### 1. Design for Round Display

The M5Dial has a 240x240 pixel round display:
- Keep important content away from corners
- Center-align main elements
- Use circular layouts where possible

### 2. Memory Management

ESP32-S3 has limited RAM:
- Keep UI simple
- Reuse components
- Use built-in fonts when possible
- Compress large images

### 3. File Organization

- **One screen per XML file**
- **One component per XML file**
- Use descriptive names: `lamp_control.xml`, not `screen2.xml`

### 4. Styling

- Define colors in `globals.xml` as constants
- Create reusable styles in `globals.xml`
- Use semantic names: `PRIMARY_COLOR` not `BLUE`

### 5. Version Control

- Commit XML files (they're text-based)
- `.gitignore` generated `*_gen.c` and `*_gen.h` files (optional)
- Keep user files (`*.c`, `*.h` without `_gen`) in version control

## Troubleshooting

### Problem: Editor won't open project

**Solution**: Make sure you're opening the `ui/` folder, not the root project folder.

### Problem: Export fails

**Solution**: 
1. Check that export path is set correctly in `project.xml`
2. Ensure `src/ui_generated/` folder exists
3. Check file permissions

### Problem: Build errors after export

**Solution**:
1. Make sure you uncommented `#define USE_GENERATED_UI`
2. Check that all generated files are present
3. Clean and rebuild: `pio run --target clean && pio run`

### Problem: UI looks different on device

**Solution**:
1. Verify fonts are available (check build flags in `platformio.ini`)
2. Check color depth settings
3. Ensure display driver is correctly configured

### Problem: Events not working

**Solution**:
1. Make sure you're calling `lv_timer_handler()` in `loop()`
2. Verify event callbacks are registered
3. Check that objects have correct IDs

### Problem: Input (dial/button) not responding

**Solution**:
1. Input handling needs to be connected to LVGL events
2. See `main.cpp` for encoder integration examples
3. Consider using LVGL's built-in input device drivers

## Next Steps

1. **Download LVGL Pro Editor** from https://pro.lvgl.io
2. **Open the `ui/` folder** in the editor
3. **Edit the provided XML templates** to customize your UI
4. **Export to C code** when ready
5. **Switch to Generated UI mode** in `main.cpp`
6. **Test on your M5Dial device**

## Resources

- **LVGL Pro Editor Docs**: https://docs.lvgl.io/master/details/xml/
- **LVGL Documentation**: https://docs.lvgl.io/
- **Online Viewer**: https://viewer.lvgl.io (test without installing)
- **Example Projects**: https://github.com/lvgl/lvgl_editor/tree/main/examples
- **M5Dial Hardware**: https://docs.m5stack.com/en/core/M5Dial

## Support

- Project Issues: See `agents.md` for project requirements
- LVGL Questions: https://forum.lvgl.io/
- M5Dial Questions: https://community.m5stack.com/

---

**Happy designing!** 🎨

The XML-based workflow makes it easy to iterate on your UI design without touching C code. Once you have a design you like, the exported code integrates seamlessly with your Home Assistant control logic.
