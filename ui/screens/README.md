# Screens

Create XML files here that start with a `<screen>` tag.

Screens are top-level containers for complete UI views in your application.

## Example

```xml
<?xml version="1.0" encoding="UTF-8"?>
<screen>
    <view width="240" height="240" flex_flow="column" flex_align="center center">
        <lv_label text="My Screen" align="center"/>
    </view>
</screen>
```

Each XML file becomes a screen that can be loaded in your application using the generated `screen_name_create()` function.
