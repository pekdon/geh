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
 * $Id: file_fetch_img.c 59 2006-08-10 09:52:56Z me@pekdon.net $
 *
 * Routines for extracting image links from html documents.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <glib/gstdio.h>
#include <ctype.h>
#include <string.h>

#include "file_fetch_img.h"
#include "file_multi.h"
#include "util.h"

static gchar *file_fetch_img_build_url (const gchar *base,
                                        const gchar *relative,
                                        const gchar *src);
static gchar *file_fetch_img_get_img (GString *buf);
static gchar *file_fetch_img_get_base (const gchar *uri);
static gchar *file_fetch_img_get_relative (const gchar *uri);

/**
 * Extract urls for images in file (html).
 *
 * @param file struct file_multi to extract links from.
 * @return GList containing list of URLs, NULL if none.
 */
GList*
file_fetch_img_extract_links (struct file_multi *file)
{
  gint c;
  gchar *img, *img_url, *base, *relative;
  FILE *fd;
  GList *urls = NULL;
  GString *buf;

  g_assert (file);

  /* Open input */
  fd = g_fopen (file_multi_get_path (file), "r");
  if (! fd) {
    g_warning ("unable to open %s for reading", file_multi_get_path (file));
    return NULL;
  }

  /* Get base and relative paths */
  base = file_fetch_img_get_base (file_multi_get_uri (file));
  relative = file_fetch_img_get_relative (file_multi_get_uri (file));

  /* Read input */
  while ((c = fgetc (fd)) != EOF) {
    /* < I M G , start of image tag */
    if (c == '<'
        && (toupper(fgetc(fd)) == 'I')
        && (toupper(fgetc(fd)) == 'M')
        && (toupper(fgetc(fd)) == 'G')) {
      /* Create buffer starting with <img */
      buf = g_string_new ("<img");

      /* Read to end of tag */
      while ((c = fgetc(fd)) != EOF) {
        buf = g_string_append_c (buf, (gchar) c);

        if (c == '>') {
          break;
        }
      }

      /* Get image from buffer */
      img = file_fetch_img_get_img (buf);
      if (img) {
        img_url = file_fetch_img_build_url (base, relative, img);
        urls = g_list_append (urls, img_url);
        g_free (img);
      }

      /* Clean up */
      g_string_free (buf, TRUE /* free_segment */);
    }
  }

  /* Cleanup */
  fclose (fd);
  g_free (base);
  g_free (relative);

  return urls;
}

/**
 * Builds complete url for src value.
 *
 * @param base Base value of URL.
 * @param relative Relative value of URL (current dir)
 * @return Complete URL for src value.
 */
gchar*
file_fetch_img_build_url (const gchar *base, const gchar *relative,
                          const gchar *src)
{
  gchar *img_url;

  if (src[0] == '/') {
    /* Absolute URL on site */
    img_url = g_strjoin(NULL, base, src, NULL);

  } else if ((util_stripos (src, "http://") == src)
             ||(util_stripos (src, "https://") == src)) {
    /* Absolute URL, just copy */
    img_url = g_strdup (src);

  } else {
    /* Relative path */
    img_url = g_strjoin (NULL, relative, "/", src, NULL);
  }

  return img_url;
}

/**
 * Gets single image from <img ... > tag.
 *
 * @param buf GString buffer reprsenting <img ... > tag.
 * @return Content of src in img tag, NULL on error.
 */
gchar*
file_fetch_img_get_img (GString *buf)
{
  gchar *img = NULL;
  guint len;
  const gchar *start, *end, *end_str;

  /* Get src tag */
  start = util_stripos (buf->str, "src=");
  if (start) {
    /* Skip src= */
    start += 4;

    /* Check separator of tag */
    if (start[0] == '\'') {
      start++;
      end_str = "'";
    } else if (start[0] == '"') {
      start++;
      end_str = "\"";
    } else {
      end_str = " ";
    }

    /* Get end of value */
    end = strstr (start, end_str);
    if (! end) {
      /* Did not find end, broken tag or ended with > */
      end = buf->str + buf->len - 1;
    }

    /* Extract src value */
    len = (end - start) / sizeof (end[0]);
    img = g_malloc (sizeof(gchar) * len);
    img = g_strndup (start, len);
  }

  return img;
}

/**
 * Builds base (host) value of URL.
 *
 * @param uri Uri to build value from.
 * @return Full URI.
 */
gchar*
file_fetch_img_get_base (const gchar *uri)
{
  gchar *base;
  const gchar *end;

  /* Skip http:// */
  end = uri + 8;

  /* Find first /, if it does not exist assume complete string */
  end = strchr (end, '/');
  if (end) {
    base = g_strndup (uri, (end - uri) / sizeof (uri[0]));
  } else {
    base = g_strdup (uri);
  }

  return base;
}

/**
 * Builds relative value of URL (current dir)
 *
 * @param uri Uri to build value from.
 * @return Full URI.
 */
gchar*
file_fetch_img_get_relative (const gchar *uri)
{
  gchar *base;
  const gchar *end;

  /* Find first /, if it does not exist assume complete string */
  end = strrchr (uri, '/');
  if (end) {
    base = g_strndup (uri, (end - uri) / sizeof (uri[0]));
  } else {
    base = g_strdup (uri);
  }

  return base;
}
