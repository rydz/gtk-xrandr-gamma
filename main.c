#include <gtk/gtk.h>

// TODO: preserve slider states when changing monitors.

#define SLIDER_MAX 100

typedef struct application_state {
  GMutex m;
  gchar* display_name;
  GArray* monitors; // GArray<*char>

  GtkLabel* command_label;

  // range 0-100
  int r;
  int g;
  int b;
  int brightness;
} application_state;

typedef struct slider_callback_data {
  application_state* state;      // application state
  int*               scalar;     // scalar value to change when slider updates
} slider_callback_data;

// update xrandr settings with application state
void
update_xrandr (application_state* state)
{
  g_mutex_lock (&state->m);
  printf ("update_xrandr: %d %d %d\n", state->r, state->g, state->b);

  gchar* data = g_strdup_printf ("xrandr --output %s --gamma %.3f:%.3f:%.3f --brightness %.3f",
                                 state->display_name, 
                                 !state->r ? 0.001 : (double)state->r  / SLIDER_MAX, 
                                 !state->g ? 0.001 : (double)state-> g / SLIDER_MAX, 
                                 !state->b ? 0.001 : (double)state->b  / SLIDER_MAX,
                                 !state->brightness ? 0.01 : (double)state->brightness / SLIDER_MAX);
  printf ("update_xrandr: %s\n", data);

  // TODO: improve this
  system(data);

  gchar* text = g_strdup_printf("command: %s", data);
  gtk_label_set_text(state->command_label, text);

  free(text);
  free(data);
  g_mutex_unlock (&state->m);
}

/**
 * @data: slider_callback_data*
 */
void
slider_callback (GtkRange*   range,
                 gpointer    data)
{
  slider_callback_data* sdata = (slider_callback_data*) data;

  g_mutex_lock (&sdata->state->m);
  *sdata->scalar = gtk_range_get_value (range);

  printf ("slider_callback: value changed: %f\n", gtk_range_get_value (range));
  printf ("%d %d %d\n",
          sdata->state->r, 
          sdata->state->g, 
          sdata->state->b);
  g_mutex_unlock (&sdata->state->m);

  update_xrandr (sdata->state);
}

void
monitor_select_callback (GtkRadioButton* btn,
                         gpointer        data)
{
  application_state* sdata = (application_state*)data;
  g_mutex_lock(&sdata->m);

  gboolean     toggled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (btn));
  const gchar* label = gtk_button_get_label (GTK_BUTTON (btn));

  printf ("monitor_select_callback: %s -- %s\n",
          label, 
          toggled ? "true" : "false");

  sdata->display_name = (char*) label;
        

  g_mutex_unlock(&sdata->m);
}

void
create_monitor_select (GtkContainer* container, 
                       application_state* state)
{
  char** mon = (char**)state->monitors->data;

  GtkWidget* group = NULL;
  for (int i=0; i < state->monitors->len; i++) {
    group = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (group), mon[i]);
    gtk_container_add(container, group);

    g_signal_connect (GTK_RADIO_BUTTON (group), 
                      "toggled", 
                      G_CALLBACK (monitor_select_callback),
                      state);
  }
}

/**
 * @scalar: pointer to the scalar value for the slider to modify
 *          when it updates, the callback will refresh the xrandr settings
 *          with the updated application state
 * 
 * create a slider for values between 0 and 1 representing gamma.
 */
void
create_slider (GtkContainer*      container,
               application_state* state,
               const gchar*       label,
               int*               scalar)
{
  GtkWidget*     wlabel;
  GtkWidget*     scale;
  GtkWidget*     hcontainer;
  GtkAdjustment* adjustment;

  hcontainer = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_box_set_spacing (GTK_BOX (hcontainer), 10);

  wlabel = gtk_label_new(label);
  gtk_widget_set_size_request( GTK_WIDGET (wlabel), 100, 10);
  gtk_container_add (GTK_CONTAINER (hcontainer), GTK_WIDGET (wlabel));

  adjustment = gtk_adjustment_new(100, // value
                                  0,   // lower
                                  101, // upper
                                  1,   // step_increment
                                  1,   // page_increment
                                  1);  // page_size

  scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adjustment);
  gtk_widget_set_hexpand (GTK_WIDGET (scale), 1);
  gtk_container_add (GTK_CONTAINER (hcontainer), GTK_WIDGET (scale));

  // TODO: free memory when the widget is destroyed.
  // currently uneeded because slider exists for the lifetime of the application
  slider_callback_data* params = (slider_callback_data*) malloc(sizeof(slider_callback_data));
  slider_callback_data _p = {
    state:  state,
    scalar: scalar
  };
  *params = _p;

  g_signal_connect (GTK_RANGE (scale), 
                    "value-changed", 
                    G_CALLBACK (slider_callback),
                    params);

  gtk_container_add(GTK_CONTAINER (container), hcontainer);
}

void
create_sliders (GtkContainer*      container,
                application_state* data)
{

  create_slider (container,
                 data,
                 "Brightness",
                 &data->brightness);

  create_slider (container,
                 data,
                 "Red",
                 &data->r);

  create_slider (container,
                 data,
                 "Green",
                 &data->g);

  create_slider (container,
                 data,
                 "Blue",
                 &data->b);
}

static void
activate (GtkApplication* app,
          gpointer        user_data)
{
  GtkWidget* window;
  GtkWidget* layout;
  GtkWidget* label;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "xrandr gamma");
  gtk_window_set_default_size (GTK_WINDOW (window), 800, 500);

  layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  create_monitor_select (GTK_CONTAINER (layout), (application_state*) user_data);
  create_sliders (GTK_CONTAINER (layout), (application_state*) user_data);
  
  label = gtk_label_new("command: ");
  gtk_container_add (GTK_CONTAINER (layout), label);
  gtk_label_set_selectable( GTK_LABEL (label), TRUE);
  ((application_state*)user_data)->command_label = GTK_LABEL (label);


  gtk_container_add( GTK_CONTAINER (window), GTK_WIDGET (layout));
  gtk_widget_show_all (window);
}

void
enum_monitors(GArray* dst)
{
  GdkDisplay* display;

  display = gdk_display_get_default();
  int n = gdk_display_get_n_monitors(display);

  for (int i=0; i < n; i++) {
    GdkMonitor* monitor = gdk_display_get_monitor(display, i);
    const char* name = gdk_monitor_get_model(monitor);
    printf("enum_monitors: %s\n", name);
    g_array_append_val(dst, name);
  }
}

int
main (int    argc,
      char** argv)
{
  gtk_init(&argc, &argv);

  int status;
  GtkApplication* app;
  GtkCssProvider* cssp;

  GArray* monitors = g_array_new(FALSE, TRUE, sizeof(char*));
  enum_monitors (monitors);

  // -------------------------- Apply CSS -------------------------------------
  const gchar* css = "box { margin: 0px 10px 0px 10px; }\n"
                     "radiobutton { margin: 0px 0px 0px 20px; }\n";
  
  GdkScreen* screen = gdk_screen_get_default();
  GError* error = NULL;
  cssp = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (cssp, css, sizeof(char)*strlen(css), &error);
  if (error) {
    printf ("main: error loading css: %d\n%s\n", error->code, error->message);
    return 1;
  }
  gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER (cssp), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  // ----------------------------------------------------------------------------

  application_state state = {
    m: {},
    monitors: monitors,
    display_name: ((char**)monitors->data)[0],
    brightness: 100,
    r: 100,
    g: 100,
    b: 100,
  };
  g_mutex_init (&state.m);

  app = gtk_application_new ("com.github.rydz.gtk-xrandr-gamma", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), &state);
  status = g_application_run (G_APPLICATION (app), argc, argv);

  // free resources 
  g_object_unref (app);
  g_array_free(monitors, FALSE);

  return status;
}

