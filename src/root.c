/*
 * Copyright © 2006-2009 Claes Nästén <me@pekdon.net>
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
#include <cairo-xlib.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "root.h"

#define ROOT_ATOM_NAME "_XROOTPMAP_ID"

static void root_image_set_x11_background (cairo_surface_t *image);
static void root_image_free_background (void);

static void root_image_create_base (cairo_surface_t **surface, cairo_t **cr, const gchar *color);

static void root_image_create_centered (cairo_surface_t *surface, cairo_t *cr, GdkPixbuf *image);
static void root_image_create_scaled (cairo_surface_t *surface, cairo_t *cr, GdkPixbuf *image);
static void root_image_create_cropped (cairo_surface_t *surface, cairo_t *cr, GdkPixbuf *image);
static void root_image_create_filled (cairo_surface_t *surface, cairo_t *cr, GdkPixbuf *image);
static void root_image_create_tiled (cairo_surface_t *surface, cairo_t *cr, GdkPixbuf *image);

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
    cairo_t *cr;
    cairo_surface_t *background;
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
    root_image_create_base (&background, &cr, color);
        
    /* Render background */
    switch (mode) {
    case ROOT_MODE_CENTER:
       root_image_create_centered (background, cr, image);
       break;
    case ROOT_MODE_SCALE:
          root_image_create_scaled (background, cr, image);
          break;
        case ROOT_MODE_CROP:
          root_image_create_cropped (background, cr, image);
          break;
        case ROOT_MODE_FILL:
          root_image_create_filled (background, cr, image);
          break;
        case ROOT_MODE_TILE:
          root_image_create_tiled (background, cr, image);
          break;
        default:
          break;
        }

        /* Set background */
        root_image_set_x11_background (background);

    cairo_destroy (cr);
    cairo_surface_destroy (background);

    g_object_unref (image);

    return TRUE;
}

/**
 * Opens new display, copies background and closes display.
 *
 * @param image GdkPixbuf with background image.
 * @param width Width of background.
 * @param height Height of background.
 */
void
root_image_set_x11_background (cairo_surface_t *image)
{
    Display *dpy;
    Window root;
    Pixmap pix;
    cairo_surface_t *surface;
    cairo_t *cr;
    guint width = cairo_image_surface_get_width(image);
    guint height = cairo_image_surface_get_width(image);

    g_assert (image);

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
    surface = cairo_xlib_surface_create (dpy, pix, DefaultVisual(dpy, 0), width, height);
    cr = cairo_create (surface);
    cairo_set_source_surface (cr, image, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    

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
 */
void
root_image_create_base (cairo_surface_t **surface_ret, cairo_t **cr_ret, const gchar *color_name)
{
    GdkScreen *screen;
    cairo_t *cr;
    cairo_surface_t *background;
    guint width, height;
    GdkColor color = {0};

    screen = gdk_screen_get_default ();

    /* Get screen size */
    width = gdk_screen_get_width (screen);
    height = gdk_screen_get_height (screen);

    /* Create pixbuf */
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
           "creating background base %dx%d", width, height);

    background = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    g_assert (background);
    cr = cairo_create (background);
    g_assert (cr);

    /* Fill background with color */
    if (! gdk_color_parse (color_name, &color)) {
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
               "failed to parse color %s", color_name);
    }

    /* Set background to parsed pixel or black if parsing failed. */
    cairo_rectangle (cr, 0.0, 0.0, width, height);
    cairo_set_source_rgb (cr, color.red, color.green, color.blue);
    cairo_fill (cr);
    cairo_paint (cr);

    *surface_ret = background;
    *cr_ret = cr;
}

/**
 * Draws image centered on dest.
 */
void
root_image_create_centered (cairo_surface_t *surface, cairo_t *cr, GdkPixbuf *image)
{
    gint src_x = 0, src_y = 0;
    gint dst_x = 0, dst_y = 0;
    guint src_width = gdk_pixbuf_get_width (image);
    guint src_height = gdk_pixbuf_get_height (image);
    guint dst_width = cairo_image_surface_get_width (surface);
    guint dst_height = cairo_image_surface_get_height (surface);

    /* Make sure copying fits into destination. */
    if (src_width > dst_width) {
        src_x = (src_width - dst_width) / 2;
    } else {
        dst_x = (dst_width - src_width) / 2;
    }
    if (src_height > dst_height) {
        src_y = (src_height - dst_height) / 2;
    } else {
        dst_y = (dst_height - src_height) / 2;
    }

    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
           "copying image of size %dx%d+%d+%d centered on %dx%d+%d+%d",
           src_width, src_height, src_x, src_y,
           dst_width, dst_height, dst_x, dst_y);

    gdk_cairo_set_source_pixbuf (cr, image, src_x, src_y);
    cairo_rectangle (cr, dst_x, dst_y, MIN(dst_width, src_width), MIN(dst_height, src_height));
    cairo_fill (cr);
}

/**
 * Draws image centered on dest with image scaled to fit.
 */
void
root_image_create_scaled (cairo_surface_t *surface, cairo_t *cr, GdkPixbuf *image)
{
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
           "scale background mode is not yet supported");
}

/**
 * Draws image on dest with image scaled+cropped to fill area.
 *
 * @param dest GdkPixbuf destination.
 * @param image Pointer to GdkPixbuf to use as source for creating background.
 */
void
root_image_create_cropped (cairo_surface_t *surface, cairo_t *cr, GdkPixbuf *image)
{
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
           "cropped background mode is not yet supported");
}

/**
 * Draws image on dest with image scaled to fill area.
 */
void
root_image_create_filled (cairo_surface_t *surface, cairo_t *cr, GdkPixbuf *image)
{
    guint src_width = gdk_pixbuf_get_width (image);
    guint src_height = gdk_pixbuf_get_height (image);
    guint dst_width = cairo_image_surface_get_width (surface);
    guint dst_height = cairo_image_surface_get_height (surface);

    cairo_scale (cr, (gdouble) dst_width / src_width, (gdouble) dst_height / src_height);
    gdk_cairo_set_source_pixbuf (cr, image, 0, 0);
    cairo_rectangle (cr, 0, 0, dst_width, dst_height);
    cairo_fill (cr);
}

/**
 * Draws image on dest image tiled to fill area.
 */
void
root_image_create_tiled (cairo_surface_t *surface, cairo_t *cr, GdkPixbuf *image)
{
    guint dst_width = cairo_image_surface_get_width (surface);
    guint dst_height = cairo_image_surface_get_height (surface);

    gdk_cairo_set_source_pixbuf (cr, image, 0, 0);
    cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
    cairo_rectangle (cr, 0, 0, dst_width, dst_height);
    cairo_fill (cr);
}
