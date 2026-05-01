/*
 * hanoi_towel_gtk.c
 * GTK 4 GUI for the Tower of Hanoi puzzle.
 *
 * Compile:
 *   gcc hanoi_towel_gtk.c -o hanoi_gtk $(pkg-config --cflags --libs gtk4)
 *
 * All original logic (hanoi, move_disk, peg arrays) is preserved unchanged.
 * Only the terminal draw/delay/clear have been replaced by GTK drawing.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_DISKS 10          /* practical GUI limit */
#define PEGS      3
#define DELAY_MS  600         /* ms between animated moves */

int pegs[PEGS][MAX_DISKS];
int tops[PEGS];
int num_disks;

static GtkWidget *drawing_area = NULL;
static GtkWidget *spin_button   = NULL;
static GtkWidget *start_button  = NULL;
static GtkWidget *status_label  = NULL;

typedef struct { int from; int to; } Move;

static Move  move_list[1 << MAX_DISKS];   /* 2^(10)-1 = 1023 max moves */
static int   move_count  = 0;
static int   move_cursor = 0;
static guint timer_id    = 0;

static const double DISK_COLORS[MAX_DISKS][3] = {   // it is for more visual confort. these colors will help to concretely distinguish the disk
    {0.90, 0.20, 0.20},   /* red      */
    {0.95, 0.55, 0.10},   /* orange   */
    {0.95, 0.85, 0.10},   /* yellow   */
    {0.20, 0.75, 0.25},   /* green    */
    {0.10, 0.60, 0.90},   /* blue     */
    {0.40, 0.20, 0.80},   /* indigo   */
    {0.80, 0.20, 0.75},   /* violet   */
    {0.00, 0.80, 0.70},   /* teal     */
    {0.85, 0.40, 0.60},   /* pink     */
    {0.50, 0.75, 0.20},   /* lime     */
};


/* Record a move instead of animating it immediately */
static void move_disk(int from, int to) {
    int disk = pegs[from][tops[from] - 1];
    pegs[from][tops[from] - 1] = 0;
    tops[from]--;

    pegs[to][tops[to]] = disk;
    tops[to]++;

    move_list[move_count].from = from;
    move_list[move_count].to   = to;
    move_count++;
}

static void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        move_disk(from, to);
    } else {
        hanoi(n - 1, from, aux, to);
        move_disk(from, to);
        hanoi(n - 1, aux, to, from);
    }
}


static void on_draw(GtkDrawingArea *area, cairo_t *cr,
                    int width, int height, gpointer data)
{
    /* background */
    cairo_set_source_rgb(cr, 0.12, 0.12, 0.18);
    cairo_paint(cr);

    if (num_disks == 0) return;

    /* layout constants */
    const double margin      = 20.0;
    const double peg_spacing = (width - 2 * margin) / (double)PEGS;
    const double peg_x[3]   = {
        margin + peg_spacing * 0.5,
        margin + peg_spacing * 1.5,
        margin + peg_spacing * 2.5,
    };
    const double ground_y    = height - 40.0;
    const double disk_height = 18.0;
    const double max_disk_w  = peg_spacing * 0.85;
    const double min_disk_w  = 24.0;
    const double peg_h       = (disk_height + 4) * MAX_DISKS + 10;

    /* draw pegs */
    for (int p = 0; p < PEGS; p++) {
        /* pole */
        cairo_set_source_rgb(cr, 0.70, 0.65, 0.50);
        cairo_set_line_width(cr, 6.0);
        cairo_move_to(cr, peg_x[p], ground_y);
        cairo_line_to(cr, peg_x[p], ground_y - peg_h);
        cairo_stroke(cr);

        /* base */
        cairo_set_source_rgb(cr, 0.55, 0.50, 0.38);
        cairo_rectangle(cr, peg_x[p] - max_disk_w / 2 - 4,
                        ground_y, max_disk_w + 8, 10);
        cairo_fill(cr);

        /* peg label */
        cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 13.0);
        char label[8];
        snprintf(label, sizeof(label), "Peg %d", p + 1);
        cairo_text_extents_t te;
        cairo_text_extents(cr, label, &te);
        cairo_move_to(cr, peg_x[p] - te.width / 2, ground_y + 28);
        cairo_show_text(cr, label);
    }

    /* draw disks */
    for (int p = 0; p < PEGS; p++) {
        for (int level = 0; level < tops[p]; level++) {
            int size = pegs[p][level];
            double ratio    = (double)size / num_disks;
            double dw       = min_disk_w + ratio * (max_disk_w - min_disk_w);
            double dy       = ground_y - (level + 1) * (disk_height + 4);
            double rx       = peg_x[p] - dw / 2.0;
            int    ci       = size - 1;          /* colour index */

            /* disk body */
            cairo_set_source_rgb(cr,
                DISK_COLORS[ci][0],
                DISK_COLORS[ci][1],
                DISK_COLORS[ci][2]);

            /* rounded rect */
            double r = disk_height / 2.0;
            cairo_new_path(cr);
            cairo_arc(cr, rx + r,      dy + r, r,  G_PI,      3 * G_PI / 2);
            cairo_arc(cr, rx + dw - r, dy + r, r, -G_PI / 2,  0);
            cairo_arc(cr, rx + dw - r, dy + disk_height - r, r, 0, G_PI / 2);
            cairo_arc(cr, rx + r,      dy + disk_height - r, r, G_PI / 2, G_PI);
            cairo_close_path(cr);
            cairo_fill(cr);

            /* highlight */
            cairo_set_source_rgba(cr, 1, 1, 1, 0.25);
            cairo_rectangle(cr, rx + 4, dy + 3, dw - 8, 5);
            cairo_fill(cr);

            /* disk number */
            cairo_set_source_rgb(cr, 0.10, 0.10, 0.10);
            cairo_set_font_size(cr, 11.0);
            char num[4];
            snprintf(num, sizeof(num), "%d", size);
            cairo_text_extents_t te2;
            cairo_text_extents(cr, num, &te2);
            cairo_move_to(cr,
                peg_x[p] - te2.width / 2,
                dy + disk_height / 2.0 + te2.height / 2.0);
            cairo_show_text(cr, num);
        }
    }

    /* move counter overlay */
    cairo_set_source_rgba(cr, 1, 1, 1, 0.55);
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);
    char info[64];
    snprintf(info, sizeof(info), "Move %d / %d", move_cursor, move_count);
    cairo_move_to(cr, margin, 22);
    cairo_show_text(cr, info);
}

/// this is for animation

static gboolean replay_next_move(gpointer user_data)
{
    if (move_cursor >= move_count) {
        /* finished */
        gtk_label_set_text(GTK_LABEL(status_label), "✔  Puzzle solved!");
        gtk_widget_set_sensitive(start_button, TRUE);
        gtk_widget_set_sensitive(spin_button,  TRUE);
        timer_id = 0;
        gtk_widget_queue_draw(drawing_area);
        return G_SOURCE_REMOVE;
    }

    Move m = move_list[move_cursor];

    /* apply the move to the live peg state */
    int disk = pegs[m.from][tops[m.from] - 1];
    pegs[m.from][tops[m.from] - 1] = 0;
    tops[m.from]--;
    pegs[m.to][tops[m.to]] = disk;
    tops[m.to]++;

    move_cursor++;
    gtk_widget_queue_draw(drawing_area);

    char msg[64];
    snprintf(msg, sizeof(msg), "Moving disk %d  ·  Peg %d → Peg %d",
             disk, m.from + 1, m.to + 1);
    gtk_label_set_text(GTK_LABEL(status_label), msg);

    return G_SOURCE_CONTINUE;
}


static void on_start_clicked(GtkButton *btn, gpointer data)
{
    /* stop any running animation */
    if (timer_id) {
        g_source_remove(timer_id);
        timer_id = 0;
    }

    num_disks = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button));

    /* reset peg state */
    tops[0] = num_disks;
    tops[1] = 0;
    tops[2] = 0;
    for (int i = 0; i < num_disks; i++)
        pegs[0][i] = num_disks - i;
    for (int p = 1; p < PEGS; p++)
        for (int i = 0; i < MAX_DISKS; i++)
            pegs[p][i] = 0;

    /* collect all moves */
    move_count  = 0;
    move_cursor = 0;
    hanoi(num_disks, 0, 2, 1);   /* ← original hanoi call, unchanged */

    /* reset peg state for replay (hanoi() moves the arrays while collecting) */
    tops[0] = num_disks;
    tops[1] = 0;
    tops[2] = 0;
    for (int i = 0; i < num_disks; i++)
        pegs[0][i] = num_disks - i;
    for (int p = 1; p < PEGS; p++)
        for (int i = 0; i < MAX_DISKS; i++)
            pegs[p][i] = 0;

    move_cursor = 0;

    gtk_widget_set_sensitive(start_button, FALSE);
    gtk_widget_set_sensitive(spin_button,  FALSE);

    char msg[64];
    snprintf(msg, sizeof(msg),
             "Solving %d disks  ·  %d total moves…", num_disks, move_count);
    gtk_label_set_text(GTK_LABEL(status_label), msg);

    gtk_widget_queue_draw(drawing_area);

    timer_id = g_timeout_add(DELAY_MS, replay_next_move, NULL);
}

// this is for the application
static void on_activate(GtkApplication *app, gpointer data)
{
   
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Tower of Hanoi");
    gtk_window_set_default_size(GTK_WINDOW(window), 720, 520);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

    /* Root vertical box */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top   (vbox, 10);
    gtk_widget_set_margin_bottom(vbox, 10);
    gtk_widget_set_margin_start (vbox, 10);
    gtk_widget_set_margin_end   (vbox, 10);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    /* Title label  */
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span font='16' weight='bold'>🗼  Tower of Hanoi</span>");
    gtk_box_append(GTK_BOX(vbox), title);

    /*  Controls row  */
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(hbox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(vbox), hbox);

    GtkWidget *lbl = gtk_label_new("Number of disks:");
    gtk_box_append(GTK_BOX(hbox), lbl);

    spin_button = gtk_spin_button_new_with_range(1, MAX_DISKS, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), 3);
    gtk_box_append(GTK_BOX(hbox), spin_button);

    start_button = gtk_button_new_with_label("▶  Start");
    gtk_box_append(GTK_BOX(hbox), start_button);
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);

    /* Drawing area  */
    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_vexpand(drawing_area, TRUE);
    gtk_widget_set_hexpand(drawing_area, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area),
                                   on_draw, NULL, NULL);
    gtk_box_append(GTK_BOX(vbox), drawing_area);

    /* Status label */
    status_label = gtk_label_new("Set the number of disks and press Start.");
    gtk_label_set_xalign(GTK_LABEL(status_label), 0.5);
    gtk_box_append(GTK_BOX(vbox), status_label);

    gtk_window_present(GTK_WINDOW(window));
}


int main(int argc, char *argv[])
{
    GtkApplication *app =
        gtk_application_new("com.example.hanoi", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
