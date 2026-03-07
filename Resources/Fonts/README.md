Place `JetBrainsMono-Regular.ttf` in this folder to bundle the font inside the plugin binary.

When present, CMake picks it up via `juce_add_binary_data`, and the UI loads it from `BinaryData` at runtime.
