This folder contains patches applied against the libraries

Current patches:

sdl_shortcut_key_passthrough.diff - patch to allow window shortcut keys to work in SDL
sdl_mac_keep_opengl_context.diff - reverts SDL hg rev d041dcb1951d so we can save OpenGL context (not sure why SDL maintainers disabled it - it seems to work fine)

