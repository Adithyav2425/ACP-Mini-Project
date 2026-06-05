# 2D Graphics Editor

A **menu-driven 2D Graphics Editor** written in C using:
- A **2D `char` array** (60 × 25 cells) as the drawing canvas
- `_` (underscore) for background, `*` (asterisk) for shape pixels
- **Windows Console API** for coloured, interactive menus and live rendering

---

## Shapes Supported

| Shape | Drawing Algorithm |
|---|---|
| **Line** | Bresenham's Line Algorithm |
| **Rectangle** | 4 × Bresenham sides |
| **Circle** | Midpoint Circle Algorithm |
| **Triangle** | 3 × Bresenham sides |

## Operations
| Operation | Description |
|---|---|
| **Add** | Pick a shape type → enter parameters → appears instantly |
| **Delete** | Enter shape ID → shape removed and canvas redrawn |
| **Modify** | Enter shape ID → re-enter all parameters → updated |
| **List** | View all visible shapes and their IDs |
| **Clear** | Wipe canvas and reset all objects |

---

## Building

### Requirements
- GCC (MinGW on Windows)
- Standard C library only — **no external dependencies**

### Compile
```bash
gcc -Wall -Wextra -std=c99 -o graphics_editor.exe graphics_editor.c -lm
```
or using Make:
```bash
make
```

### Run
```bash
graphics_editor.exe
```
or:
```bash
make run
```

---

## Controls

### Main Menu
| Key | Action |
|---|---|
| `↑` / `↓` | Navigate items |
| `Enter` | Select |
| `1` – `6` | Direct shortcuts |
| `q` / `ESC` | Exit |

### Add Shape Sub-menu
| Key | Shape |
|---|---|
| `a` | Line |
| `b` | Rectangle |
| `c` | Circle |
| `d` | Triangle |
| `e` / `ESC` | Back |

### Coordinate System
| Axis | Range | Direction |
|---|---|---|
| **Col** (x) | 0 – 59 | Left → Right |
| **Row** (y) | 0 – 24 | Top → Bottom |

---

## Example Shapes to Try

| Shape | Parameters |
|---|---|
| Horizontal line | x1=0 y1=12 x2=59 y2=12 |
| Diagonal line | x1=0 y1=0 x2=59 y2=24 |
| Full rectangle | x1=2 y1=1 x2=57 y2=23 |
| Small circle | cx=30 cy=12 r=8 |
| Upright triangle | (30,1) (5,23) (55,23) |

---

## File Structure

```
Mini Project/
├── graphics_editor.c   ← Complete source code
├── Makefile            ← Build script
└── README.md           ← This file
```

---

## Screen Layout

```
+------------------------------------------------------------+[ GRAPHICS EDITOR    ]
|                      CANVAS (60x25)                        |  1. Add Shape
|  ____________________________________________________________|  2. Delete Shape
|  ____________...___________________________________________  |  3. Modify Shape
|  ...           (canvas content with * for shapes)           |  4. List Objects
|  ____________________________________________________________|  5. Clear Canvas
+------------------------------------------------------------+  6. Exit
  Status bar message here
```
