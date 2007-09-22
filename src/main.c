/*
 * Copyright (c) 2006 Claes Nästén <me@pekdon.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * $Id: main.c 75 2006-09-20 15:48:45Z me@pekdon.net $
 *
 * Main routine, startup and option parsing.
 */

#ifdef HAVE_CONFIG_H_H
#include "config.h"
#endif /* HAVE_CONFIG_H_H */

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "geh.h"

#include "dir.h"
#include "file_fetch.h"
#include "file_multi.h"
#include "file_queue.h"
#include "ui_window.h"
#include "root.h"

/* Initialize options */
struct _options options = {
  NULL /* file_list */,
  -2, /* mode */
  NULL /* mode_str */,
  0 /* timeout */,
  FALSE /* win_nodecor */,
  720 /* win_width */,
  740 /* win_height */,
  FALSE /* root */,
  ROOT_MODE_CENTER /* root_mode */,
  NULL /* root_mode_str */,
  "#000000" /* root_color */,
  128 /* thumb_side */,
  FALSE /* recursive */,
  -1 /* levels */,
  NULL /* files */
};

/**
 * Command line parsing structure.
 */
static GOptionEntry cmdopt[] = {
  {"background", 'b', 0, G_OPTION_ARG_NONE, &options.root, "Background mode"},
  {"color", 'c', 0, G_OPTION_ARG_STRING, &options.root_color, "Background color"},
  {"height", 'H', 0, G_OPTION_ARG_INT, &options.win_height, "Window height"},
  {"levels", 'l', 0, G_OPTION_ARG_INT, &options.levels, "Levels of recursion"},
  {"mode", 'm', 0, G_OPTION_ARG_STRING, &options.mode_str, "Image display mode"},
  {"nodecor", 'n', 0, G_OPTION_ARG_NONE, &options.win_nodecor, "No decor for window"},
  {"recursive", 'r', 0, G_OPTION_ARG_NONE, &options.recursive, "Recursive directory scanning"},
  {"set", 's', 0, G_OPTION_ARG_STRING, &options.root_mode_str, "Background mode"},
  {"thumbside", 't', 0, G_OPTION_ARG_INT, &options.thumb_side, "Thumbnail size in pixels"},
  {"timeout", 'T', 0, G_OPTION_ARG_INT, &options.timeout, "Display window for seconds"},
  {"width", 'W', 0, G_OPTION_ARG_INT, &options.win_width, "Window width"},
  { G_OPTION_REMAINING, ' ', 0, G_OPTION_ARG_FILENAME_ARRAY, &options.files, "" },
  { NULL }
};

static void main_print_usage (void);
static gboolean main_timeout_quit (gpointer data);

/**
 * Main routine
 */
int
main (int argc, char *argv[])
{
  gint file_count = 0;
  gboolean stop = FALSE;

  GList *it;
  GOptionContext *context;

  struct ui_window *ui;
  struct dir_scan *dir_scan;
  struct file_multi *file;
  struct file_fetch *file_fetch;
  struct file_queue *file_queue;

  /* Parse command line options */
  context = g_option_context_new ("");
  g_option_context_add_main_entries (context, cmdopt, NULL);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  g_option_context_parse (context, &argc, &argv, NULL);
  g_option_context_free (context);

  /* Make sure there is something to do (need input files) */
  if (options.files) {
    /* Count entries in order to determine mode */
    for (file_count = 0; options.files[file_count] != NULL; file_count++)
          ;
  } else {
    main_print_usage ();
    return 1;
  }

  /* Start with init threading, gdk, i18n and gtk */
  if (!g_thread_supported ()) {
    g_thread_init (NULL);
  }

  gdk_threads_init ();

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  /* Get mode to use */
  if (options.mode_str) {
    if (! g_strcasecmp ("THUMB", options.mode_str)) {
      options.mode = UI_WINDOW_MODE_THUMB;
    } else if (! g_strcasecmp ("FULL", options.mode_str)) {
      options.mode = UI_WINDOW_MODE_FULL;
    } else if (! g_strcasecmp ("SLIDE", options.mode_str)) {
      options.mode = UI_WINDOW_MODE_SLIDE;
    } else if (! g_strcasecmp ("ROOT", options.mode_str)) {
      options.mode = -1;
    } else {
      g_warning ("Invalid mode %s", options.mode_str);
      return 1;
    }
  }

  if (options.mode != -1) {
    /* Create UI window */
    ui = ui_window_new ();

    /* Fallback mode check */
    if (! options.mode_str) {
      if (file_count > 1) {
        options.mode = UI_WINDOW_MODE_SLIDE;
      } else {
        options.mode = UI_WINDOW_MODE_FULL;
      }
    }

    ui_window_show (ui);
    ui_window_progress_set_total (ui, file_count);
    ui_window_set_mode (ui, options.mode);

    /* Scan dirs and fetch files that is added to the thumbnail view.
       The file queue is created with one reference owned by the dir
       scanner. */
    file_queue = file_queue_new (1);
    dir_scan = dir_scan_start (file_queue, options.files,
                               &ui_window_progress_add, (gpointer) ui);
    file_fetch = file_fetch_start (file_queue, options.file_list, ui);

    if (options.timeout > 0) {
        g_timeout_add (options.timeout * 1000, main_timeout_quit, NULL);
    }

    /* Enter main loop */
    gdk_threads_enter ();
    gtk_main ();
    gdk_threads_leave ();

    /* Cleanup after fetching of files */
    dir_scan_stop (dir_scan);
    file_fetch_stop (file_fetch);

    /* Free UI after stopping of scanning as it uses UI */
    ui_window_free (ui);

    for (it = file_queue_get_list (file_queue); it; it = g_list_next (it)) {
      file_multi_close ((struct file_multi*) it->data);
    }
    file_queue_free (file_queue);

  } else {
    file = file_multi_open (options.files[0]);
    if (! file_multi_need_fetch (file) || file_multi_fetch (file, &stop)) {
      /* Root mode */
      root_image_set_image (file, options.root_color, options.root_mode);
    } else {
      g_warning ("Failed to fetch %s", argv[argc -1 ]);
    }
    file_multi_close (file);
  }

  return 0;
}

/**
 * Print usage information.
 */
void
main_print_usage (void)
{
  g_fprintf (stderr, "Usage: %s [-bclmrst] [FILE]...\n", PACKAGE_NAME);
  g_fprintf (stderr, "Display images and set background image\n");
}

/**
 * Timeout function to stop geh.
 *
 * @param data Not used
 * @return FALSE
 */
gboolean
main_timeout_quit(gpointer data)
{
    gtk_main_quit ();
    return FALSE;
}
