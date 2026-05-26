/*
 * XFCE4 CPU Monitor Panel Plugin
 * Monitors: CPU usage %, temperature
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-macros.h>

#define PLUGIN_NAME   "xfce4-cpu-plugin"
#define UPDATE_MS     900
#define BORDER        2
#define TEXT_MAX      512

#define PROC_STAT     "/proc/stat"
#define HWMON_BASE    "/sys/class/hwmon"

typedef struct {
    XfcePanelPlugin *plugin;

    GtkWidget       *ebox;
    GtkWidget       *box;
    GtkWidget       *icon;
    GtkWidget       *label;

    guint            timer_id;

    gchar           *temp_path;
    gchar           *freq_path;

    gulong           prev_total;
    gulong           prev_idle;

    gint             cpu_usage;
    gint             freq_mhz;
    gint             temp_mc;
} CpuPlugin;

static gchar *try_read (const gchar *path)
{
    FILE *f;
    static gchar buf[256];

    if (!path)
        return NULL;
    f = fopen (path, "r");
    if (!f)
        return NULL;
    if (fgets (buf, sizeof(buf), f))
    {
        g_strstrip (buf);
        fclose (f);
        return buf;
    }
    fclose (f);
    return NULL;
}

static void detect_temp_sensor (CpuPlugin *cpu)
{
    gchar *path;
    gchar *name;
    int hwmon;

    for (hwmon = 0; hwmon < 16; hwmon++)
    {
        path = g_strdup_printf (HWMON_BASE "/hwmon%d/name", hwmon);
        name = try_read (path);
        g_free (path);

        if (name && g_strcmp0 (name, "coretemp") == 0)
        {
            path = g_strdup_printf (HWMON_BASE "/hwmon%d/temp1_input", hwmon);
            if (try_read (path))
            {
                cpu->temp_path = path;
                return;
            }
            g_free (path);
        }
    }

    for (hwmon = 0; hwmon < 16; hwmon++)
    {
        path = g_strdup_printf (HWMON_BASE "/hwmon%d/name", hwmon);
        name = try_read (path);
        g_free (path);

        if (name && (g_strcmp0 (name, "k10temp") == 0 ||
                     g_strcmp0 (name, "zenpower") == 0))
        {
            path = g_strdup_printf (HWMON_BASE "/hwmon%d/temp1_input", hwmon);
            if (try_read (path))
            {
                cpu->temp_path = path;
                return;
            }
            g_free (path);
        }
    }
}

static void read_cpu_data (CpuPlugin *cpu)
{
    gchar *stat;
    gulong user, nice, system, idle, iowait, irq, softirq, steal;
    gulong total, diff_total, diff_idle;
    gchar *val;

    stat = try_read (PROC_STAT);
    if (!stat)
        return;

    g_strstrip (stat);

    if (sscanf (stat, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
                &user, &nice, &system, &idle,
                &iowait, &irq, &softirq, &steal) < 4)
        return;

    total = user + nice + system + idle + iowait + irq + softirq + steal;

    if (cpu->prev_total > 0)
    {
        diff_total = total - cpu->prev_total;
        diff_idle = idle - cpu->prev_idle;

        if (diff_total > 0)
            cpu->cpu_usage = (gint) (100.0 * (diff_total - diff_idle) / diff_total);
        else
            cpu->cpu_usage = 0;
    }

    cpu->prev_total = total;
    cpu->prev_idle = idle;

    if (cpu->temp_path)
    {
        val = try_read (cpu->temp_path);
        cpu->temp_mc = val ? (gint) strtol (val, NULL, 10) / 1000 : 0;
    }

    if (cpu->freq_path)
    {
        val = try_read (cpu->freq_path);
        cpu->freq_mhz = val ? (gint) strtol (val, NULL, 10) / 1000 : 0;
    }
}

static const gchar *temp_color (gint temp)
{
    if (temp >= 75) return "#ff4444";
    if (temp >= 55) return "#ffaa00";
    return "#44cc44";
}

static const gchar *usage_color (gint usage)
{
    if (usage >= 90) return "#ff4444";
    if (usage >= 60) return "#ffaa00";
    return "#44cc44";
}

static void update_display (CpuPlugin *cpu)
{
    gchar markup[TEXT_MAX];
    gchar tooltip[512];

    g_snprintf (markup, sizeof(markup),
                "<span>CPU: "
                "<span foreground=\"%s\">%d%%</span> | "
                "<span foreground=\"%s\">%d" "\xc2\xb0" "C</span></span>",
                usage_color (cpu->cpu_usage),
                cpu->cpu_usage,
                temp_color (cpu->temp_mc),
                cpu->temp_mc);

    gtk_label_set_markup (GTK_LABEL (cpu->label), markup);

    g_snprintf (tooltip, sizeof(tooltip),
                "CPU\n"
                "Usage:     %d%%\n"
                "Clock:     %.1f GHz\n"
                "Temp:      %d" "\xc2\xb0" "C",
                cpu->cpu_usage,
                (double) cpu->freq_mhz / 1000.0,
                cpu->temp_mc);

    gtk_widget_set_tooltip_text (GTK_WIDGET (cpu->plugin), tooltip);
}

static gboolean cpu_plugin_update (gpointer user_data)
{
    CpuPlugin *cpu = (CpuPlugin *) user_data;

    read_cpu_data (cpu);
    update_display (cpu);
    return TRUE;
}

static gboolean cpu_plugin_size_changed (XfcePanelPlugin *plugin,
                                          gint size, CpuPlugin *cpu)
{
    GtkOrientation orientation = xfce_panel_plugin_get_orientation (plugin);

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
    else
        gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

    return TRUE;
}

static void cpu_plugin_orientation_changed (XfcePanelPlugin *plugin,
                                             GtkOrientation orientation,
                                             CpuPlugin *cpu)
{
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_orientable_set_orientation (GTK_ORIENTABLE (cpu->box),
                                         GTK_ORIENTATION_HORIZONTAL);
        gtk_label_set_angle (GTK_LABEL (cpu->label), 0);
    }
    else
    {
        gtk_orientable_set_orientation (GTK_ORIENTABLE (cpu->box),
                                         GTK_ORIENTATION_VERTICAL);
        gtk_label_set_angle (GTK_LABEL (cpu->label), 90);
    }
}

static void cpu_plugin_free (XfcePanelPlugin *plugin, CpuPlugin *cpu)
{
    if (cpu->timer_id)
        g_source_remove (cpu->timer_id);

    g_free (cpu->temp_path);
    g_free (cpu->freq_path);

    gtk_widget_destroy (cpu->ebox);
    g_free (cpu);
}

static void cpu_plugin_construct (XfcePanelPlugin *plugin)
{
    CpuPlugin *cpu;
    GtkOrientation orientation;

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    cpu = g_new0 (CpuPlugin, 1);
    cpu->plugin = plugin;

    g_signal_connect (G_OBJECT (plugin), "free-data",
                      G_CALLBACK (cpu_plugin_free), cpu);
    g_signal_connect (G_OBJECT (plugin), "size-changed",
                      G_CALLBACK (cpu_plugin_size_changed), cpu);
    g_signal_connect (G_OBJECT (plugin), "orientation-changed",
                      G_CALLBACK (cpu_plugin_orientation_changed), cpu);

    detect_temp_sensor (cpu);

    if (try_read ("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"))
        cpu->freq_path = g_strdup ("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");

    orientation = xfce_panel_plugin_get_orientation (plugin);

    cpu->ebox = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (plugin), cpu->ebox);

    cpu->box = gtk_box_new (
        orientation == GTK_ORIENTATION_VERTICAL
            ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_container_add (GTK_CONTAINER (cpu->ebox), cpu->box);

    cpu->icon = gtk_image_new_from_icon_name ("indicator-sensors-cpu",
                                               GTK_ICON_SIZE_MENU);
    gtk_box_pack_start (GTK_BOX (cpu->box), cpu->icon, FALSE, FALSE, 0);

    cpu->label = gtk_label_new ("CPU ...");
    gtk_widget_set_name (cpu->label, "xfce4-cpu-plugin-label");
    gtk_box_pack_start (GTK_BOX (cpu->box), cpu->label, FALSE, FALSE, 0);

    gtk_container_set_border_width (GTK_CONTAINER (cpu->ebox), BORDER);

    if (orientation == GTK_ORIENTATION_VERTICAL)
        gtk_label_set_angle (GTK_LABEL (cpu->label), 90);

    gtk_widget_show_all (GTK_WIDGET (plugin));

    read_cpu_data (cpu);
    cpu_plugin_update (cpu);
    cpu->timer_id = g_timeout_add (UPDATE_MS, cpu_plugin_update, cpu);
}

XFCE_PANEL_PLUGIN_REGISTER (cpu_plugin_construct);
