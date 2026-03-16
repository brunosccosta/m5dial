# Components

Create XML files here that start with a `<component>` tag.

Components are reusable UI elements that can have custom properties and can be used across multiple screens.

## Example

```xml
<?xml version="1.0" encoding="UTF-8"?>
<component>
    <api>
        <prop name="button_text" type="string" default="Click Me"/>
        <prop name="button_color" type="color" default="0x0000FF"/>
    </api>
    
    <view extends="lv_button" bg_color="$button_color">
        <lv_label text="$button_text" align="center"/>
    </view>
</component>
```

Use components in screens or other components by their filename (without .xml extension).
