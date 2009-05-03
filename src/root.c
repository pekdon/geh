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
 * Background image setting routines.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "root.h"

#define ROOT_ATOM_NAME "_XROOTPMAP_ID"

static void root_image_set_background (GdkPixbuf *image);
static void root_image_set_x11_background (Pixmap x11_pix,
                                           guint width, guint height);
static void root_image_free_background (void);

static GdkPixbuf *root_image_create_base (const gchar *color);

static void root_image_create_centered (GdkPixbuf *dest, GdkPixbuf *image);
static void root_image_create_scaled (GdkPixbuf *dest, GdkPixbuf *image);
static void root_image_create_cropped (GdkPixbuf *dest, GdkPixbuf *image);
static void root_image_create_filled (GdkPixbuf *dest, GdkPixbuf *image);
static void root_image_create_tiled (GdkPixbuf *dest, GdkPixbuf *image);

/**
 * Sets background image.
 *
 * @param file File to set as background image.
 * @param color Color for background on centered and scaled modes.
 * @param mode Mode to set.
 * @return TRUE on success, else FALSE.
 */
gboolean
root_image_set_image (struct file_multi *file, const gchar *color, guint mode)
{
  GdkPixbuf *image;
  GdkPixbuf *background = NULL;
  GError *err = NULL;

  g_assert (file);

  /* Default color is black */
  if (! color) {
    color = "#000000";
  }

  /* Load source file */
  image = gdk_pixbuf_new_from_file (file_multi_get_path (file), &err);
  if (!image || err) {
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, err->message);
    g_free (err);

    return FALSE;
  }

  /* Create base background */
  background = root_image_create_base (color);

  /* Render background */
  switch (mode) {
  case ROOT_MODE_CENTER:
    root_image_create_centered (background, image);
    break;
  case ROOT_MODE_SCALE:
    root_image_create_scaled (background, image);
    break;
  case ROOT_MODE_CROP:
    root_image_create_cropped (background, image);
    break;
  case ROOT_MODE_FILL:
    root_image_create_filled (background, image);
    break;
  case ROOT_MODE_TILE:
    root_image_create_tiled (background, image);
    break;
  default:
    break;
  }

  /* Set background */
  root_image_set_background (background);

  /* Cleanup */
  g_object_unref (image);
  g_object_unref (background);

  return FALSE;
}

/**
 * Sets image as background.
 *
 * @param image Pointer to GdkPixbuf to use as background.
 */
void
root_image_set_background (GdkPixbuf *image)
{
  guint width, height;
  GdkScreen *screen;
  GdkPixmap *pixmap;
  GdkGC *gc;

  g_assert (image);

  screen = gdk_screen_get_default ();
  width = gdk_pixbuf_get_width (image);
  height = gdk_pixbuf_get_height (image);

  /* Free up resources */
  root_image_free_background ();

  /* Create background */
  pixmap = gdk_pixmap_new (gdk_screen_get_root_window (screen),
                           width, height,
                           gdk_screen_get_system_visual (screen)->depth);
  g_assert (pixmap);

  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
         "rendering background of size %dx%d to pixmap", width, height);

  gc = gdk_gc_new (pixmap);
  gdk_draw_pixbuf (pixmap, gc, image,
                   0 /* src_x */, 0 /* src_y */, 0 /* dest_x */, 0 /* dest_y */,
                   width, height, 
                   GDK_RGB_DITHER_MAX, 0 /* x_dither */, 0 /* y_dither */);
  g_object_unref (gc);

  root_image_set_x11_background (GDK_PIXMAP_XID (pixmap), width, height);
}

/**
 * Opens new display, copies background and closes display.
 *
 * @param x11_pix Pixmap to use copy and use as background.
 * @param width Width of background.
 * @param height Height of background.
 */
void
root_image_set_x11_background (Pixmap x11_pix, guint width, guint height)
{
  Display *dpy;
  Window root;
  Pixmap pix;

  /* Open up a new display to make sure that only the pixmap and
     nothing else is saved when XSetCloseDownMode to RetainPermanent */
  dpy = XOpenDisplay (NULL);
  if (!dpy) {
    g_error ("unable to reopen display");
  }

  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
         "setting background of size %dx%d", width, height);

  /* Get default root window and create background pixmap. */
  root = RootWindow (dpy, DefaultScreen (dpy));
  pix = XCreatePixmap (dpy, root, width, height,
                       DefaultDepth (dpy, gdk_x11_get_default_screen ()));

  /* Copy background to resource on newly opened display. */
  XCopyArea (dpy, x11_pix, pix, DefaultGC (dpy, DefaultScreen (dpy)),
             0 /* src_x */, 0 /* src_y */, width, height,
             0 /* dest_x */, 0 /* dest_y */);

  /* Set property so resources can be freed. */
  XChangeProperty (dpy, root, XInternAtom (dpy, ROOT_ATOM_NAME, False),
                   XA_PIXMAP, 32, PropModeReplace, (guchar*) &pix, 1);

  /* Set background on root window */
  XSetWindowBackgroundPixmap (dpy, root, pix);

  XClearWindow (dpy, root);

  /* Set resources to stick after closing the display then close */
  XSetCloseDownMode (dpy, RetainPermanent);

  /* Sync and close display */
  XSync (dpy, FALSE);
  XCloseDisplay (dpy);
}

/**
 * Frees up resources used by previously set background.
 */
void
root_image_free_background (void)
{
  guchar *data;
  gint actual_format, actual_length;
  GdkAtom atom, actual_atom;

  /* Get atom used for tracking the root pixmap. */
  atom = gdk_atom_intern (ROOT_ATOM_NAME, TRUE);
  if (atom == GDK_NONE) {
    /* If the atom does not exists, it can not be set and thus there
       can not be any resources to free. */
    return;
  }

  /* Get property */
  if (gdk_property_get (gdk_get_default_root_window (),
                        atom, _GDK_MAKE_ATOM (XA_PIXMAP), 0L, 1L, FALSE,
                        &actual_atom, &actual_format, &actual_length,
                        &data))  {
    if (actual_atom == _GDK_MAKE_ATOM (XA_PIXMAP)) {
      /* Free up resources */
      XKillClient (gdk_x11_get_default_xdisplay (), *((Pixmap*) data));

    } else {
      g_warning ("mismatching root atom type");
    }
  }
}

/**
 * Creates image with color of screen size.
 *
 * @param color Name of color.
 * @return New GdkPixbuf of screen size.
 */
GdkPixbuf*
root_image_create_base (const gchar *color_name)
{
  GdkScreen *screen;
  GdkPixbuf *background;
  GdkColor color;
  guint width, height;
  guint32 pixel = 0;

  screen = gdk_screen_get_default ();

  /* Get screen size */
  width = gdk_screen_get_width (screen);
  height = gdk_screen_get_height (screen);

  /* Create pixbuf */
  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
         "creating background base %dx%d", width, height);
  background = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE /* alpha */,
                               8 /* bps */, width, height);
  g_assert (background);

  /* Fill background with color */
  if (gdk_color_parse (color_name, &color)) {
    if (gdk_colormap_alloc_color (gdk_colormap_get_system (), &color,
                                  TRUE /* writeable */ , TRUE /* match */ )) {
      pixel = color.pixel;
    }
  } else {
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
           "failed to parse color %s", color_name);
  }

  /* Set background to parsed pixel or black if parsing failed. */
  gdk_pixbuf_fill (background, pixel);

  return background;
}

/**
 * Draws image centered on dest.
 *
 * @param dest GdkPixbuf destination.
 * @param image Pointer to GdkPixbuf to use as source for drawing background.
 */
void
root_image_create_centered (GdkPixbuf *dest, GdkPixbuf *image)
{
  gint src_x = 0, src_y = 0;
  gint dest_x = 0, dest_y = 0;
  guint src_width = gdk_pixbuf_get_width (image);
  guint src_height = gdk_pixbuf_get_height (image);
  guint dest_width = gdk_pixbuf_get_width (dest);
  guint dest_height = gdk_pixbuf_get_height (dest);

  /* Make sure copying fits into destination. */
  if (src_width > dest_width) {
      src_x = (src_width - dest_width) / 2;
  } else {
      dest_x = (dest_width - src_width) / 2;
  }
  if (src_height > dest_height) {
      src_y = (src_height - dest_height) / 2;
  } else {
      dest_y = (dest_height - src_height) / 2;
  }

  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
         "copying image of size %dx%d+%d+%d centered on %dx%d+%d+%d",
         src_width, src_height, src_x, src_y,
         dest_width, dest_height, dest_x, dest_y);

  gdk_pixbuf_copy_area (image, src_x, src_y,
                        MIN(dest_width, src_width), MIN(dest_height, src_height),
                        dest, dest_x, dest_y);
}

/**
 * Draws image centered on dest with image scaled to fit.
 *
 * @param dest GdkPixbuf destination.
 * @param image Pointer to GdkPixbuf to use as source for creating background.
 */
void
root_image_create_scaled (GdkPixbuf *dest, GdkPixbuf *image)
{
  guint src_width = gdk_pixbuf_get_width (image);
  guint src_height = gdk_pixbuf_get_height (image);
  guint dest_width = gdk_pixbuf_get_width (dest);
  guint dest_height = gdk_pixbuf_get_height (dest);

  gdk_pixbuf_scale (image, dest,
                    0 /* dest_x */, 0 /* dest_y */, dest_width, dest_height,
                    0 /* offset_x */, 0  /* offset_y */,
                    (gdouble) dest_width / src_width,
                    (gdouble) dest_height / src_height,
                    GDK_INTERP_BILINEAR);
}

/**
 * Draws image on dest with image scaled+cropped to fill area.
 *
 * @param dest GdkPixbuf destination.
 * @param image Pointer to GdkPixbuf to use as source for creating background.
 */
void
root_image_create_cropped (GdkPixbuf *dest, GdkPixbuf *image)
{
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
           "cropped background mode is not yet supported");
}

/**
 * Draws image on dest with image scaled to fill area.
 *
 * @param dest GdkPixbuf destination.
 * @param image Pointer to GdkPixbuf to use as source for creating background.
 */
void
root_image_create_filled (GdkPixbuf *dest, GdkPixbuf *image)
{
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
           "filled background mode is not yet supported");
}

/**
 * Draws image on dest image tiled to fill area.
 *
 * @param dest GdkPixbuf destination.
 * @param image Pointer to GdkPixbuf to use as source for creating background.
 */
void
root_image_create_tiled (GdkPixbuf *dest, GdkPixbuf *image)
{
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
           "filled background mode is not yet supported");
}
