/*
 * ============================================================
 *  2D Graphics Editor — Menu-driven (Windows Console API)
 *  Canvas  : 2D char array  (CANVAS_ROWS x CANVAS_COLS)
 *  Background : '_'   |   Shape pixels : '*'
 *
 *  Shapes     : Line, Rectangle, Circle, Triangle
 *  Operations : Add, Delete, Modify, List, Clear
 *
 *  Build  :  gcc -o graphics_editor graphics_editor.c
 *  Run    :  graphics_editor.exe
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <conio.h>          /* _getch(), _kbhit() */
#include <windows.h>        /* Console API          */

/* ─── Canvas dimensions ─────────────────────────────────────────── */
#define CANVAS_ROWS  25
#define CANVAS_COLS  60
#define MAX_OBJECTS  100

/* ─── Console colour attributes ─────────────────────────────────── */
#define COL_RESET      FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
#define COL_BACKGROUND (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define COL_SHAPE      (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COL_TITLE      (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COL_HIGHLIGHT  (BACKGROUND_GREEN | BACKGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define COL_MENU_ITEM  (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define COL_STATUS     (BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#define COL_DIM        (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define COL_PROMPT     (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)

/* ─── Key codes ──────────────────────────────────────────────────── */
#define KEY_UP_ARROW   72
#define KEY_DOWN_ARROW 80
#define KEY_ENTER_CODE 13
#define KEY_ESC        27
#define KEY_BACKSPACE  8
#define SPECIAL_KEY    0   /* prefix for arrow keys from _getch()    */

/* ─── Shape types ────────────────────────────────────────────────── */
typedef enum {
    SHAPE_LINE = 0,
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE
} ShapeType;

/* ─── Shape descriptor ───────────────────────────────────────────── */
typedef struct {
    int       id;
    ShapeType type;
    int       visible;

    /* LINE / RECTANGLE : (x1,y1)-(x2,y2)                          */
    /* TRIANGLE         : three vertices                             */
    /* CIRCLE           : centre (cx,cy), radius r                  */
    int x1, y1, x2, y2, x3, y3;
    int cx, cy, r;
} Shape;

/* ─── Globals ────────────────────────────────────────────────────── */
static char  canvas[CANVAS_ROWS][CANVAS_COLS];
static Shape objects[MAX_OBJECTS];
static int   obj_count = 0;
static int   next_id   = 1;

static HANDLE hCon;   /* stdout console handle */

/* ═══════════════════════════════════════════════════════════════════
 *  CONSOLE HELPERS
 * ═══════════════════════════════════════════════════════════════════ */

static void con_goto(int x, int y)
{
    COORD c = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hCon, c);
}

static void con_color(WORD attr)
{
    SetConsoleTextAttribute(hCon, attr);
}

static void con_hide_cursor(void)
{
    CONSOLE_CURSOR_INFO ci = { 1, FALSE };
    SetConsoleCursorInfo(hCon, &ci);
}

static void con_show_cursor(void)
{
    CONSOLE_CURSOR_INFO ci = { 25, TRUE };
    SetConsoleCursorInfo(hCon, &ci);
}

static void con_cls(void)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count, written;
    COORD home = {0, 0};
    GetConsoleScreenBufferInfo(hCon, &csbi);
    count = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(hCon, ' ', count, home, &written);
    FillConsoleOutputAttribute(hCon, csbi.wAttributes, count, home, &written);
    SetConsoleCursorPosition(hCon, home);
}

/* Print a string at (x,y) with given colour */
static void con_print(int x, int y, WORD attr, const char *s)
{
    con_goto(x, y);
    con_color(attr);
    printf("%s", s);
}

/* Draw a horizontal box line */
static void con_hline(int x, int y, int len, WORD attr)
{
    char buf[256];
    int i;
    if (len <= 0 || len >= (int)sizeof(buf)) return;
    for (i = 0; i < len; i++) buf[i] = '-';
    buf[len] = '\0';
    con_print(x, y, attr, buf);
}

/* ═══════════════════════════════════════════════════════════════════
 *  CANVAS PRIMITIVES
 * ═══════════════════════════════════════════════════════════════════ */

static void put_pixel(int row, int col)
{
    if (row >= 0 && row < CANVAS_ROWS && col >= 0 && col < CANVAS_COLS)
        canvas[row][col] = '*';
}

static void init_canvas(void)
{
    int r, c;
    for (r = 0; r < CANVAS_ROWS; r++)
        for (c = 0; c < CANVAS_COLS; c++)
            canvas[r][c] = '_';
}

/* ─── Bresenham's Line ──────────────────────────────────────────── */
static void draw_line_pixels(int x1, int y1, int x2, int y2)
{
    int dx  = abs(x2 - x1);
    int dy  = abs(y2 - y1);
    int sx  = (x1 < x2) ? 1 : -1;
    int sy  = (y1 < y2) ? 1 : -1;
    int err = dx - dy, e2;

    while (1) {
        put_pixel(y1, x1);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
}

/* ─── Rectangle ─────────────────────────────────────────────────── */
static void draw_rectangle_pixels(int x1, int y1, int x2, int y2)
{
    draw_line_pixels(x1, y1, x2, y1);
    draw_line_pixels(x1, y2, x2, y2);
    draw_line_pixels(x1, y1, x1, y2);
    draw_line_pixels(x2, y1, x2, y2);
}

/* ─── Midpoint Circle ───────────────────────────────────────────── */
static void draw_circle_pixels(int cx, int cy, int r)
{
    int x = 0, y = r, d = 1 - r;
    while (x <= y) {
        put_pixel(cy + y, cx + x);  put_pixel(cy - y, cx + x);
        put_pixel(cy + y, cx - x);  put_pixel(cy - y, cx - x);
        put_pixel(cy + x, cx + y);  put_pixel(cy - x, cx + y);
        put_pixel(cy + x, cx - y);  put_pixel(cy - x, cx - y);
        if (d < 0)     d += 2 * x + 3;
        else         { d += 2 * (x - y) + 5; y--; }
        x++;
    }
}

/* ─── Triangle ──────────────────────────────────────────────────── */
static void draw_triangle_pixels(int x1, int y1, int x2, int y2,
                                   int x3, int y3)
{
    draw_line_pixels(x1, y1, x2, y2);
    draw_line_pixels(x2, y2, x3, y3);
    draw_line_pixels(x3, y3, x1, y1);
}

/* ─── Rasterise one shape ───────────────────────────────────────── */
static void rasterise(const Shape *s)
{
    if (!s->visible) return;
    switch (s->type) {
    case SHAPE_LINE:
        draw_line_pixels(s->x1, s->y1, s->x2, s->y2); break;
    case SHAPE_RECTANGLE:
        draw_rectangle_pixels(s->x1, s->y1, s->x2, s->y2); break;
    case SHAPE_CIRCLE:
        draw_circle_pixels(s->cx, s->cy, s->r); break;
    case SHAPE_TRIANGLE:
        draw_triangle_pixels(s->x1, s->y1, s->x2, s->y2, s->x3, s->y3); break;
    }
}

static void redraw_all(void)
{
    int i;
    init_canvas();
    for (i = 0; i < obj_count; i++) rasterise(&objects[i]);
}

/* ═══════════════════════════════════════════════════════════════════
 *  DISPLAY
 *  Layout (console columns / rows):
 *
 *   Col 0               Col 61     Col 63
 *   |<-- CANVAS_COLS -->|  |<-- MENU (28 cols) -->|
 *   row 0  : top border
 *   row 1..CANVAS_ROWS : canvas content
 *   row CANVAS_ROWS+1  : bottom border
 *   row CANVAS_ROWS+2  : status bar (full width)
 * ═══════════════════════════════════════════════════════════════════ */

#define CANVAS_ORIGIN_X  1
#define CANVAS_ORIGIN_Y  1
#define MENU_ORIGIN_X    (CANVAS_COLS + 3)
#define MENU_WIDTH       28

/* Draw the static box around the canvas */
static void draw_frame(void)
{
    int r;
    char buf[CANVAS_COLS + 2];

    /* Top border */
    buf[0] = '+';
    memset(buf + 1, '-', CANVAS_COLS);
    buf[CANVAS_COLS + 1] = '+';
    buf[CANVAS_COLS + 2] = '\0';
    con_print(0, 0, COL_TITLE, buf);
    con_print(1, 0, COL_TITLE, " 2D CANVAS (col 0-59, row 0-24) ");

    /* Side borders */
    for (r = 0; r < CANVAS_ROWS; r++) {
        con_print(0, r + CANVAS_ORIGIN_Y, COL_DIM, "|");
        con_print(CANVAS_COLS + 1, r + CANVAS_ORIGIN_Y, COL_DIM, "|");
    }

    /* Bottom border */
    con_print(0, CANVAS_ROWS + CANVAS_ORIGIN_Y, COL_TITLE, buf);

    /* Menu border */
    con_print(MENU_ORIGIN_X - 1, 0, COL_TITLE, "+");
    for (r = 0; r <= CANVAS_ROWS + 1; r++) {
        con_print(MENU_ORIGIN_X - 1, r, COL_DIM, "|");
    }
}

/* Render canvas content */
static void display_canvas(void)
{
    int r, c;
    for (r = 0; r < CANVAS_ROWS; r++) {
        con_goto(CANVAS_ORIGIN_X, r + CANVAS_ORIGIN_Y);
        for (c = 0; c < CANVAS_COLS; c++) {
            if (canvas[r][c] == '*') {
                con_color(COL_SHAPE);
                putchar('*');
            } else {
                con_color(COL_DIM);
                putchar('_');
            }
        }
    }
    con_color(COL_RESET);
}

/* Status bar at bottom */
static void set_status(const char *msg)
{
    char buf[100];
    snprintf(buf, sizeof(buf), "  %-88s", msg);
    con_print(0, CANVAS_ROWS + CANVAS_ORIGIN_Y + 1, COL_STATUS, buf);
}

/* ═══════════════════════════════════════════════════════════════════
 *  MENU RENDERING
 * ═══════════════════════════════════════════════════════════════════ */

static const char *shape_name(ShapeType t)
{
    switch (t) {
    case SHAPE_LINE:      return "Line";
    case SHAPE_RECTANGLE: return "Rect";
    case SHAPE_CIRCLE:    return "Circ";
    case SHAPE_TRIANGLE:  return "Tri ";
    default:              return "????";
    }
}

/* Clear menu area */
static void clear_menu_area(void)
{
    int r;
    char blank[MENU_WIDTH + 2];
    memset(blank, ' ', MENU_WIDTH + 1);
    blank[MENU_WIDTH + 1] = '\0';
    for (r = 0; r <= CANVAS_ROWS + 1; r++)
        con_print(MENU_ORIGIN_X, r, COL_RESET, blank);
}

static void draw_menu_title(const char *title)
{
    char buf[MENU_WIDTH + 1];
    snprintf(buf, sizeof(buf), "[ %-*s]", MENU_WIDTH - 3, title);
    con_print(MENU_ORIGIN_X, 0, COL_TITLE, buf);
    con_hline(MENU_ORIGIN_X, 1, MENU_WIDTH, COL_DIM);
}

static void draw_main_menu(int highlight)
{
    const char *items[] = {
        "1. Add Shape",
        "2. Delete Shape",
        "3. Modify Shape",
        "4. List Objects",
        "5. Clear Canvas",
        "6. Exit"
    };
    int n = 6, i;
    char buf[MENU_WIDTH + 1];

    clear_menu_area();
    draw_menu_title("GRAPHICS EDITOR");

    for (i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "  %-*s", MENU_WIDTH - 2, items[i]);
        if (i == highlight)
            con_print(MENU_ORIGIN_X, i + 2, COL_HIGHLIGHT, buf);
        else
            con_print(MENU_ORIGIN_X, i + 2, COL_MENU_ITEM, buf);
    }

    con_hline(MENU_ORIGIN_X, n + 3, MENU_WIDTH, COL_DIM);

    char cnt[40];
    snprintf(cnt, sizeof(cnt), "  Objects: %d/%d", obj_count, MAX_OBJECTS);
    con_print(MENU_ORIGIN_X, n + 4, COL_PROMPT, cnt);

    snprintf(cnt, sizeof(cnt), "  UP/DOWN + ENTER or 1-6");
    con_print(MENU_ORIGIN_X, n + 5, COL_DIM, cnt);
}

static void draw_add_submenu(int highlight)
{
    const char *items[] = {
        "a. Line",
        "b. Rectangle",
        "c. Circle",
        "d. Triangle",
        "e. < Back"
    };
    int n = 5, i;
    char buf[MENU_WIDTH + 1];

    clear_menu_area();
    draw_menu_title("ADD SHAPE");

    for (i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "  %-*s", MENU_WIDTH - 2, items[i]);
        if (i == highlight)
            con_print(MENU_ORIGIN_X, i + 2, COL_HIGHLIGHT, buf);
        else
            con_print(MENU_ORIGIN_X, i + 2, COL_MENU_ITEM, buf);
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  INPUT HELPERS
 * ═══════════════════════════════════════════════════════════════════ */

/* Read an integer from the menu area at given menu-relative row.
   Returns 1 on success, 0 on cancel (ESC / empty).               */
static int menu_read_int(int menu_row, const char *prompt, int *val)
{
    char buf[16];
    int  i = 0, ch;

    /* Print prompt */
    char label[MENU_WIDTH + 1];
    snprintf(label, sizeof(label), "  %-*s", MENU_WIDTH - 2, prompt);
    con_print(MENU_ORIGIN_X, menu_row, COL_PROMPT, label);

    /* Position cursor after prompt text */
    int prompt_len = (int)strlen(prompt) + 2;
    con_show_cursor();
    con_goto(MENU_ORIGIN_X + prompt_len, menu_row);
    con_color(COL_MENU_ITEM);

    while (1) {
        ch = _getch();

        if (ch == KEY_ENTER_CODE) break;
        if (ch == KEY_ESC) { con_hide_cursor(); return 0; }

        if ((ch == KEY_BACKSPACE) && i > 0) {
            i--;
            buf[i] = '\0';
            con_goto(MENU_ORIGIN_X + prompt_len + i, menu_row);
            con_color(COL_MENU_ITEM);
            putchar(' ');
            con_goto(MENU_ORIGIN_X + prompt_len + i, menu_row);
        } else if ((isdigit(ch) || (ch == '-' && i == 0)) && i < 14) {
            buf[i++] = (char)ch;
            buf[i]   = '\0';
            con_color(COL_MENU_ITEM);
            putchar(ch);
        }
        fflush(stdout);
    }

    con_hide_cursor();
    buf[i] = '\0';
    if (i == 0) return 0;
    *val = atoi(buf);
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════
 *  SHAPE PARAMETER FORMS  (drawn into the menu panel)
 * ═══════════════════════════════════════════════════════════════════ */

#define FORM_START 2   /* menu row where form begins */

static void form_header(const char *title)
{
    clear_menu_area();
    draw_menu_title(title);
    char hint[MENU_WIDTH + 1];
    snprintf(hint, sizeof(hint), "  Col:0-%d  Row:0-%d",
             CANVAS_COLS - 1, CANVAS_ROWS - 1);
    con_print(MENU_ORIGIN_X, FORM_START, COL_DIM, hint);
}

static int form_line(Shape *s)
{
    int x1, y1, x2, y2;
    form_header("LINE");
    if (!menu_read_int(FORM_START+1, "Start col (x1):", &x1)) return 0;
    if (!menu_read_int(FORM_START+2, "Start row (y1):", &y1)) return 0;
    if (!menu_read_int(FORM_START+3, "End   col (x2):", &x2)) return 0;
    if (!menu_read_int(FORM_START+4, "End   row (y2):", &y2)) return 0;
    s->type = SHAPE_LINE;
    s->x1 = x1; s->y1 = y1; s->x2 = x2; s->y2 = y2;
    return 1;
}

static int form_rectangle(Shape *s)
{
    int x1, y1, x2, y2;
    form_header("RECTANGLE");
    if (!menu_read_int(FORM_START+1, "Top-left  col:", &x1)) return 0;
    if (!menu_read_int(FORM_START+2, "Top-left  row:", &y1)) return 0;
    if (!menu_read_int(FORM_START+3, "Bot-right col:", &x2)) return 0;
    if (!menu_read_int(FORM_START+4, "Bot-right row:", &y2)) return 0;
    s->type = SHAPE_RECTANGLE;
    s->x1 = x1; s->y1 = y1; s->x2 = x2; s->y2 = y2;
    return 1;
}

static int form_circle(Shape *s)
{
    int cx, cy, r;
    form_header("CIRCLE");
    if (!menu_read_int(FORM_START+1, "Centre col (cx):", &cx)) return 0;
    if (!menu_read_int(FORM_START+2, "Centre row (cy):", &cy)) return 0;
    if (!menu_read_int(FORM_START+3, "Radius (r):",      &r))  return 0;
    if (r <= 0) { set_status("Radius must be > 0. Cancelled."); return 0; }
    s->type = SHAPE_CIRCLE;
    s->cx = cx; s->cy = cy; s->r = r;
    return 1;
}

static int form_triangle(Shape *s)
{
    int x1, y1, x2, y2, x3, y3;
    form_header("TRIANGLE");
    if (!menu_read_int(FORM_START+1, "Vertex1 col:", &x1)) return 0;
    if (!menu_read_int(FORM_START+2, "Vertex1 row:", &y1)) return 0;
    if (!menu_read_int(FORM_START+3, "Vertex2 col:", &x2)) return 0;
    if (!menu_read_int(FORM_START+4, "Vertex2 row:", &y2)) return 0;
    if (!menu_read_int(FORM_START+5, "Vertex3 col:", &x3)) return 0;
    if (!menu_read_int(FORM_START+6, "Vertex3 row:", &y3)) return 0;
    s->type = SHAPE_TRIANGLE;
    s->x1 = x1; s->y1 = y1;
    s->x2 = x2; s->y2 = y2;
    s->x3 = x3; s->y3 = y3;
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════
 *  OBJECT MANAGEMENT
 * ═══════════════════════════════════════════════════════════════════ */

static int find_shape(int id)
{
    int i;
    for (i = 0; i < obj_count; i++)
        if (objects[i].id == id && objects[i].visible) return i;
    return -1;
}

static void add_object(Shape s)
{
    if (obj_count >= MAX_OBJECTS) {
        set_status("ERROR: Object limit reached!");
        return;
    }
    s.id      = next_id++;
    s.visible = 1;
    objects[obj_count++] = s;
    rasterise(&objects[obj_count - 1]);
}

static int delete_object(int id)
{
    int idx = find_shape(id);
    if (idx < 0) return 0;
    objects[idx].visible = 0;
    redraw_all();
    return 1;
}

static int modify_object(int id, Shape updated)
{
    int idx = find_shape(id);
    if (idx < 0) return 0;
    updated.id      = id;
    updated.visible = 1;
    objects[idx]    = updated;
    redraw_all();
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════
 *  LIST OBJECTS PANEL
 * ═══════════════════════════════════════════════════════════════════ */

static void list_objects_panel(void)
{
    int i, row = FORM_START, vis = 0;
    char line[MENU_WIDTH + 1];

    clear_menu_area();
    draw_menu_title("OBJECT LIST");

    for (i = 0; i < obj_count && row < CANVAS_ROWS; i++) {
        Shape *s = &objects[i];
        if (!s->visible) continue;
        vis++;

        switch (s->type) {
        case SHAPE_LINE:
            snprintf(line, sizeof(line),
                     "ID%2d LINE(%d,%d)->(%d,%d)",
                     s->id, s->x1, s->y1, s->x2, s->y2);
            break;
        case SHAPE_RECTANGLE:
            snprintf(line, sizeof(line),
                     "ID%2d RECT(%d,%d)-(%d,%d)",
                     s->id, s->x1, s->y1, s->x2, s->y2);
            break;
        case SHAPE_CIRCLE:
            snprintf(line, sizeof(line),
                     "ID%2d CIRC c(%d,%d)r%d",
                     s->id, s->cx, s->cy, s->r);
            break;
        case SHAPE_TRIANGLE:
            snprintf(line, sizeof(line),
                     "ID%2d TRI(%d,%d)(%d,%d)(%d,%d)",
                     s->id,s->x1,s->y1,s->x2,s->y2,s->x3,s->y3);
            break;
        }
        con_print(MENU_ORIGIN_X, row++, COL_PROMPT, "  ");
        con_print(MENU_ORIGIN_X + 2, row - 1, COL_MENU_ITEM, line);
    }

    if (vis == 0)
        con_print(MENU_ORIGIN_X, row++, COL_DIM, "  (no objects)");

    con_hline(MENU_ORIGIN_X, row + 1, MENU_WIDTH, COL_DIM);
    con_print(MENU_ORIGIN_X, row + 2, COL_DIM, "  Press any key...");
    _getch();
}

/* ═══════════════════════════════════════════════════════════════════
 *  MINI OBJECT LIST (for delete/modify prompts)
 * ═══════════════════════════════════════════════════════════════════ */

static int show_mini_list(void)
{
    int i, row = FORM_START, vis = 0;
    char line[MENU_WIDTH + 1];

    for (i = 0; i < obj_count && row < CANVAS_ROWS - 2; i++) {
        Shape *s = &objects[i];
        if (!s->visible) continue;
        vis++;
        snprintf(line, sizeof(line), "  ID:%2d %s",
                 s->id, shape_name(s->type));
        con_print(MENU_ORIGIN_X, row++, COL_MENU_ITEM, line);
    }
    if (vis == 0) {
        con_print(MENU_ORIGIN_X, row++, COL_DIM, "  (no objects)");
    }
    return row;
}

/* ═══════════════════════════════════════════════════════════════════
 *  MENU ACTIONS
 * ═══════════════════════════════════════════════════════════════════ */

static void do_add_shape(void)
{
    int highlight = 0, ch, done = 0;

    while (!done) {
        draw_add_submenu(highlight);
        ch = _getch();

        int sel = -1;

        if (ch == 0 || ch == 0xE0) {
            /* special key */
            int ch2 = _getch();
            if (ch2 == KEY_UP_ARROW   && highlight > 0) highlight--;
            if (ch2 == KEY_DOWN_ARROW && highlight < 4) highlight++;
            continue;
        }

        switch (tolower(ch)) {
        case 'a': sel = 0; break;
        case 'b': sel = 1; break;
        case 'c': sel = 2; break;
        case 'd': sel = 3; break;
        case 'e': done = 1; break;
        case KEY_ESC: done = 1; break;
        case KEY_ENTER_CODE: sel = highlight; break;
        }

        if (done) break;
        if (sel < 0) continue;
        if (sel == 4) { done = 1; break; }

        highlight = sel;
        draw_add_submenu(highlight);

        Shape s;
        memset(&s, 0, sizeof(s));
        int ok = 0;

        switch (sel) {
        case 0: ok = form_line(&s);      break;
        case 1: ok = form_rectangle(&s); break;
        case 2: ok = form_circle(&s);    break;
        case 3: ok = form_triangle(&s);  break;
        }

        if (ok) {
            add_object(s);
            display_canvas();
            set_status("Shape added successfully! Press a key to continue.");
        } else {
            set_status("Cancelled / invalid input.");
        }
        done = 1;
    }
}

static void do_delete_shape(void)
{
    int id, row;
    clear_menu_area();
    draw_menu_title("DELETE SHAPE");
    row = show_mini_list();

    if (!menu_read_int(row + 1, "Enter Shape ID:", &id)) {
        set_status("Cancelled.");
        return;
    }

    if (delete_object(id)) {
        display_canvas();
        set_status("Shape deleted.");
    } else {
        set_status("ID not found or already deleted.");
    }
}

static void do_modify_shape(void)
{
    int id, idx, row;
    clear_menu_area();
    draw_menu_title("MODIFY SHAPE");
    row = show_mini_list();

    if (!menu_read_int(row + 1, "Enter Shape ID:", &id)) {
        set_status("Cancelled.");
        return;
    }

    idx = find_shape(id);
    if (idx < 0) {
        set_status("ID not found.");
        return;
    }

    Shape updated;
    memset(&updated, 0, sizeof(updated));
    updated.type = objects[idx].type;

    int ok = 0;
    switch (objects[idx].type) {
    case SHAPE_LINE:      ok = form_line(&updated);      break;
    case SHAPE_RECTANGLE: ok = form_rectangle(&updated); break;
    case SHAPE_CIRCLE:    ok = form_circle(&updated);    break;
    case SHAPE_TRIANGLE:  ok = form_triangle(&updated);  break;
    }

    if (ok) {
        modify_object(id, updated);
        display_canvas();
        set_status("Shape modified successfully!");
    } else {
        set_status("Cancelled / invalid input.");
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  INITIAL SPLASH SCREEN
 * ═══════════════════════════════════════════════════════════════════ */

static void show_splash(void)
{
    con_cls();
    con_color(COL_TITLE);
    con_goto(20, 8);
    printf("  +-----------------------------------------+");
    con_goto(20, 9);
    printf("  |    2D GRAPHICS EDITOR  v1.0             |");
    con_goto(20, 10);
    printf("  |    Canvas: %dx%d  |  Max Objects: %d       |",
           CANVAS_COLS, CANVAS_ROWS, MAX_OBJECTS);
    con_goto(20, 11);
    printf("  |                                         |");
    con_goto(20, 12);
    printf("  |  Shapes:  Line  Rectangle  Circle       |");
    con_goto(20, 13);
    printf("  |           Triangle                      |");
    con_goto(20, 14);
    printf("  |  Ops:     Add  Delete  Modify  List     |");
    con_goto(20, 15);
    printf("  |                                         |");
    con_goto(20, 16);
    printf("  |       Press any key to start...         |");
    con_goto(20, 17);
    printf("  +-----------------------------------------+");
    con_color(COL_RESET);
    _getch();
}

/* ═══════════════════════════════════════════════════════════════════
 *  MAIN EVENT LOOP
 * ═══════════════════════════════════════════════════════════════════ */

static void run_editor(void)
{
    int highlight = 0, ch, running = 1;

    init_canvas();
    con_cls();
    draw_frame();
    display_canvas();
    draw_main_menu(highlight);
    set_status("Welcome!  Use UP/DOWN + ENTER to select, or press 1-6.");

    while (running) {
        draw_main_menu(highlight);
        ch = _getch();

        /* Arrow keys arrive as 0 or 0xE0 followed by a code */
        if (ch == 0 || ch == 0xE0) {
            int ch2 = _getch();
            if (ch2 == KEY_UP_ARROW   && highlight > 0) highlight--;
            if (ch2 == KEY_DOWN_ARROW && highlight < 5) highlight++;
            continue;
        }

        /* Direct number shortcuts */
        if (ch >= '1' && ch <= '6') {
            highlight = ch - '1';
            ch = KEY_ENTER_CODE;
        }

        if (ch == KEY_ENTER_CODE) {
            switch (highlight) {
            case 0:
                do_add_shape();
                draw_frame();
                display_canvas();
                break;
            case 1:
                do_delete_shape();
                draw_frame();
                display_canvas();
                break;
            case 2:
                do_modify_shape();
                draw_frame();
                display_canvas();
                break;
            case 3:
                list_objects_panel();
                draw_frame();
                display_canvas();
                break;
            case 4:
                {
                    int i;
                    for (i = 0; i < obj_count; i++) objects[i].visible = 0;
                    obj_count = 0; next_id = 1;
                    init_canvas();
                    draw_frame();
                    display_canvas();
                    set_status("Canvas cleared and reset.");
                }
                break;
            case 5:
                running = 0;
                break;
            }
        }

        if (ch == KEY_ESC || ch == 'q' || ch == 'Q') running = 0;
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  ENTRY POINT
 * ═══════════════════════════════════════════════════════════════════ */

int main(void)
{
    /* Setup console */
    hCon = GetStdHandle(STD_OUTPUT_HANDLE);

    /* Enlarge console window & buffer if possible */
    COORD bufSize = { 120, 40 };
    SetConsoleScreenBufferSize(hCon, bufSize);

    SMALL_RECT winRect = { 0, 0, 119, 37 };
    SetConsoleWindowInfo(hCon, TRUE, &winRect);

    SetConsoleTitleA("2D Graphics Editor");
    con_hide_cursor();

    show_splash();
    run_editor();

    /* Restore terminal */
    con_cls();
    con_show_cursor();
    con_color(COL_RESET);
    printf("\n  2D Graphics Editor exited. Goodbye!\n\n");

    return 0;
}
