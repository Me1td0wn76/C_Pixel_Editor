# README  
This is a simple pixel art editor written in C using SDL2. It allows users to create pixel art by selecting colors from a palette and drawing on a grid-based canvas. The editor supports basic functionalities such as saving and loading artwork, as well as clearing the canvas.
## Features
- Grid-based canvas for pixel art creation.
- Color palette with a selection of colors.
- Basic drawing tools (pencil, eraser).
- Undo/redo functionality.
- Save and load artwork as BMP files.
- Clear canvas option.
## Requirements
- SDL2 library
- C compiler (e.g., GCC)
## Compilation
To compile the program, use the following command:
```bash
gcc -o pixel_art_editor C_Pixel_Art_Editor.c -lSDL2
```
## Usage
Run the compiled program:
```bash
C_pixel_art_editor.exe
```

## Controls
- Left Mouse Button: Draw on the canvas.
- Right Mouse Button: Erase on the canvas.
- Ctrl + Z: Undo.
- Ctrl + Y: Redo.
- Ctrl + S: Save artwork.
- Ctrl + O: Load artwork.
- C: Clear canvas.

## License
This project is licensed under the MIT License. See the LICENSE file for details.