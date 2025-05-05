#include <gtk/gtk.h>
#include <cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_ITERATIONS 100   // Maximum number of iterations for termination
#define EPSILON 0.001        // Small threshold to check for convergence

// Structure to store a 2D point
typedef struct {
    double x;
    double y;
} Point;

// Global Variables
Point *data_points = NULL;
int n_points;
Point *centroids = NULL;
int n_centroids;
int *assignments = NULL;
int iteration = 0;
int animation_rate = 1000; // Default animation rate in milliseconds
int step = 0; // 0 for assignment step, 1 for centroid update

int x_unit = 0;
int y_unit = 0;
int cell_unit_length = 30;
int origin_cord[2] = {425, 425}; // Real coordinate of the origin
guint timeout_id = 0; // Global variable to store the timeout ID

// Function prototypes
double* cal_rel_origin(int x, int y);
void cal_axises_unit();
void read_data(const char *filename);
void set_cluster_color(cairo_t *cr, int cluster_id);
void draw_legend(cairo_t *cr);
void draw_grid_lines(cairo_t *cr);
void draw_visualization(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data);
void update_clusters();
gboolean on_timer(gpointer user_data);
static void open_app(GApplication *app, GFile **files, int n_files, const char *hint);
static void activate(GtkApplication *app, gpointer user_data);

// Randomly initialize centroids (Lloyd-Forgy initialization)
void initialize_centroids() {
    srand(time(NULL));
    for (int i = 0; i < n_centroids; i++) {
        int random_index = rand() % n_points;
        centroids[i].x = data_points[random_index].x;
        centroids[i].y = data_points[random_index].y;
    }
}

double* cal_rel_origin(int x, int y) {
    static double result[2];
    result[0] = origin_cord[0] + (x * cell_unit_length) / x_unit;
    result[1] = origin_cord[1] - (y * cell_unit_length) / y_unit;
    return result;
}

void cal_axises_unit() {
    int biggest_x = 0;
    int biggest_y = 0;
    for (int i = 0; i < n_points; i++) {
        if (data_points[i].x > biggest_x) biggest_x = data_points[i].x;
        if (data_points[i].y > biggest_y) biggest_y = data_points[i].y;
    }
    x_unit = biggest_x / 10;
    y_unit = biggest_y / 10;
    if (x_unit == 0) x_unit = 1;
    if (y_unit == 0) y_unit = 1;
}

void read_data(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    fscanf(file, "%d", &n_points);
    data_points = (Point *)malloc(n_points * sizeof(Point));
    assignments = (int *)malloc(n_points * sizeof(int));

    for (int i = 0; i < n_points; i++) {
        fscanf(file, "%lf %lf", &data_points[i].x, &data_points[i].y);
    }

    fscanf(file, "%d", &n_centroids);
    centroids = (Point *)malloc(n_centroids * sizeof(Point));
    initialize_centroids();

    fclose(file);
}

void set_cluster_color(cairo_t *cr, int cluster_id) {
    switch (cluster_id % 5) {
        case 0: cairo_set_source_rgb(cr, 0.5, 0.0, 0.5); break; // Purple
        case 1: cairo_set_source_rgb(cr, 0.0, 1.0, 1.0); break; // Cyan
        case 2: cairo_set_source_rgb(cr, 1.0, 0.5, 0.0); break; // Orange
        case 3: cairo_set_source_rgb(cr, 0.0, 0.5, 0.0); break; // Dark Green
        case 4: cairo_set_source_rgb(cr, 0.5, 0.5, 0.0); break; // Olive
    }
}

void draw_legend(cairo_t *cr) {
    cairo_set_font_size(cr, 20.0);
    const char *labels[] = {"Cluster 0", "Cluster 1", "Cluster 2", "Cluster 3", "Cluster 4", "Centroids"};
    for (int i = 0; i < 5; i++) {
        set_cluster_color(cr, i);
        cairo_rectangle(cr, 800, 50 + i * 30, 15, 15);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_move_to(cr, 820, 65 + i * 30);
        cairo_show_text(cr, labels[i]);
    }

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Black
    cairo_rectangle(cr, 800, 50 + 5 * 30, 15, 15);
    cairo_stroke_preserve(cr);
    cairo_fill(cr);
    cairo_move_to(cr, 820, 65 + 5 * 30);
    cairo_show_text(cr, labels[5]);
}

void draw_grid_lines(cairo_t *cr) {
    cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 0.8); // Light gray with transparency
    cairo_set_line_width(cr, 1.0);
    for (int i = 0; i < 25; i++) {
        double x = 50 + i * 30;
        cairo_move_to(cr, x, 50);
        cairo_line_to(cr, x, 800);
        cairo_stroke(cr);
    }
    for (int i = 0; i < 25; i++) {
        double y = 50 + i * 30;
        cairo_move_to(cr, 50, y);
        cairo_line_to(cr, 800, y);
        cairo_stroke(cr);
    }
}

void draw_visualization(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data) {
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    draw_grid_lines(cr);

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_line_width(cr, 2.0);

    // X-axis
    cairo_move_to(cr, 50, 425);
    cairo_line_to(cr, 800, 425);
    cairo_stroke(cr);

    // Y-axis
    cairo_move_to(cr, 425, 50);
    cairo_line_to(cr, 425, 800);
    cairo_stroke(cr);

    for (int i = 0; i < n_points; i++) {
        set_cluster_color(cr, assignments[i]);
        double *pos = cal_rel_origin(data_points[i].x, data_points[i].y);
        cairo_arc(cr, pos[0], pos[1], 5.0, 0, 2 * M_PI);
        cairo_fill(cr);
    }

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_line_width(cr, 2.0);
    for (int i = 0; i < n_centroids; i++) {
        double *pos = cal_rel_origin(centroids[i].x, centroids[i].y);
        cairo_move_to(cr, pos[0] - 5, pos[1] - 5);
        cairo_line_to(cr, pos[0] + 5, pos[1] + 5);
        cairo_move_to(cr, pos[0] - 5, pos[1] + 5);
        cairo_line_to(cr, pos[0] + 5, pos[1] - 5);
        cairo_stroke(cr);
    }

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_font_size(cr, 20.0);
    char iteration_label[50];
    sprintf(iteration_label, "Iteration: %d", iteration);
    cairo_move_to(cr, 10, 30);
    cairo_show_text(cr, iteration_label);

    draw_legend(cr);
}

void update_clusters() {
    static Point *prev_centroids = NULL;
    if (!prev_centroids) {
        prev_centroids = (Point *)malloc(n_centroids * sizeof(Point));
    }

    if (step == 0) {
        // Assignment Step
        for (int i = 0; i < n_points; i++) {
            double min_distance = INFINITY;
            int closest_centroid = -1;
            for (int j = 0; j < n_centroids; j++) {
                double dx = data_points[i].x - centroids[j].x;
                double dy = data_points[i].y - centroids[j].y;
                double distance = sqrt(dx * dx + dy * dy);
                if (distance < min_distance) {
                    min_distance = distance;
                    closest_centroid = j;
                }
            }
            assignments[i] = closest_centroid;
        }
        step = 1;
    } else if (step == 1) {
        // Update Step
        Point *new_centroids = (Point *)calloc(n_centroids, sizeof(Point));
        int *counts = (int *)calloc(n_centroids, sizeof(int));

        for (int i = 0; i < n_points; i++) {
            int cluster_id = assignments[i];
            new_centroids[cluster_id].x += data_points[i].x;
            new_centroids[cluster_id].y += data_points[i].y;
            counts[cluster_id]++;
        }

        for (int i = 0; i < n_centroids; i++) {
            if (counts[i] > 0) {
                new_centroids[i].x /= counts[i];
                new_centroids[i].y /= counts[i];
            }
        }

        // Check for convergence
        int converged = 1;
        for (int i = 0; i < n_centroids; i++) {
            double dx = new_centroids[i].x - centroids[i].x;
            double dy = new_centroids[i].y - centroids[i].y;
            if (sqrt(dx * dx + dy * dy) > EPSILON) {
                converged = 0;
            }
            prev_centroids[i] = centroids[i];
            centroids[i] = new_centroids[i];
        }

        free(new_centroids);
        free(counts);

        iteration++;
        step = 0;

        // Terminate if converged or max iterations reached
        if (converged || iteration >= MAX_ITERATIONS) {
            if (timeout_id != 0) {
                g_source_remove(timeout_id); // Stop the timer
                timeout_id = 0;
            }
            free(prev_centroids);
            prev_centroids = NULL;
        }
    }
}

gboolean on_timer(gpointer user_data) {
    GtkWidget *drawing_area = GTK_WIDGET(user_data);
    update_clusters();
    gtk_widget_queue_draw(drawing_area);
    return G_SOURCE_CONTINUE;
}

static void open_app(GApplication *app, GFile **files, int n_files, const char *hint) {
    if (n_files > 0) {
        const char *filename = g_file_get_path(files[0]);
        read_data(filename);
        cal_axises_unit();
    }
    g_application_activate(app);
}

static void activate(GtkApplication *app, gpointer user_data) {
    if (!data_points || !centroids) {
        fprintf(stderr, "Error: No data loaded. Please provide a valid data file.\n");
        return;
    }

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(window), 850, 850);
    gtk_window_set_title(GTK_WINDOW(window), "K-Means Visualization");

    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(drawing_area), 1000);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(drawing_area), 1000);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area), draw_visualization, NULL, NULL);
    gtk_window_set_child(GTK_WINDOW(window), drawing_area);

    timeout_id = g_timeout_add(animation_rate, on_timer, drawing_area);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("com.example.kmeans", G_APPLICATION_HANDLES_OPEN);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    g_signal_connect(app, "open", G_CALLBACK(open_app), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    free(data_points);
    free(centroids);
    free(assignments);

    return status;
}
