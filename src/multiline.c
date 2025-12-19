#include "multiline.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>

#include <stdio.h>

#define ASSERT(x)

/* This helper function helps splitting an ustr in several delimited parts.
 * It returns an ustr that refers to the next part of the string that
 * is delimited by the delimiters in delimiter.
 * Returns NULL at the end of the string.
 * Pos is updated to byte index of character after the delimiter or
 * to the end of the string.
 */
static const ALLEGRO_USTR *ustr_split_next(const ALLEGRO_USTR *ustr,
   ALLEGRO_USTR_INFO *info, int *pos, const char *delimiter)
{
   const ALLEGRO_USTR *result;
   int end, size;

   size = al_ustr_size(ustr);
   if (*pos >= size) {
      return NULL;
   }

   end = al_ustr_find_set_cstr(ustr, *pos, delimiter);
   if (end == -1)
      end = size;

   result = al_ref_ustr(info, ustr, *pos, end);
   /* Set pos to character AFTER delimiter */
   al_ustr_next(ustr, &end);
   (*pos) = end;
   return result;
}

/* This returns the next "soft" line of text from ustr
 * that will fit in max_with using the font font, starting at pos *pos.
 * These are "soft" lines because they are broken up if needed at a space
 * or tab character.
 * This function updates pos if needed, and returns the next "soft" line,
 * or NULL if no more soft lines.
 * The soft line will not include the trailing space where the
 * line was split, but pos will be set to point to after that trailing
 * space so iteration can continue easily.
 */
static const ALLEGRO_USTR *get_next_soft_line(const ALLEGRO_USTR *ustr,
   ALLEGRO_USTR_INFO *info, int *pos,
   const ALLEGRO_FONT *font, float max_width, bool keep_first_word)
{
   const ALLEGRO_USTR *result = NULL;
   const char *whitespace = " \t";
   int old_end = 0;
   int end = 0;
   int size = al_ustr_size(ustr);
   bool first_word = keep_first_word;
   
   if (*pos >= size) {
      return NULL;
   }

   end = *pos;
   old_end = end;
   if (!keep_first_word) {
      // if we're not keeping the first word, compensate for skipping whitespace.
      old_end--;
   }

   do {
      /* On to the next word. */
      end = al_ustr_find_set_cstr(ustr, end, whitespace);
      if (end < 0)
         end = size;

      /* Reference to the line that is being built. */
      result = al_ref_ustr(info, ustr, *pos, end);

      /* Check if the line is too long. If it is, return a soft line. */
      if (al_get_ustr_width(font, result) > max_width) {
         /* Corner case: a single word may not even fit the line.
          * In that case, return the word/line anyway as the "soft line",
          * the user can set a clip rectangle to cut it. */

         if (first_word) {
            /* Set pos to character AFTER end to allow easy iteration. */
            al_ustr_next(ustr, &end);
            *pos = end;
            return result;
         }
         else {
            /* Not first word, return old end position without the new word */
            result = al_ref_ustr(info, ustr, *pos, old_end);
            /* Set pos to character AFTER end to allow easy iteration. */
            al_ustr_next(ustr, &old_end);
            *pos = old_end;
            return result;
         }
      }
      first_word = false;
      old_end    = end;
      /* Skip the character at end which normally is whitespace. */
      al_ustr_next(ustr, &end);
   } while (end < size);

   /* If we get here the whole ustr will fit.*/
   result = al_ref_ustr(info, ustr, *pos, size);
   *pos = size;
   return result;
}

/* Function: al_do_multiline_ustr
 */
void do_multiline_ustr(const ALLEGRO_FONT *font, 
   float *xflow, float *yflow, float line_height, float max_width,
   const ALLEGRO_USTR *ustr,
   bool (*cb)(int line_num, float xflow, float yflow, const ALLEGRO_USTR * line, void *extra),
   void *extra)
{
   const char *linebreak  = "\n";
   const ALLEGRO_USTR *hard_line, *soft_line;
   ALLEGRO_USTR_INFO hard_line_info, soft_line_info;
   int hard_line_pos = 0, soft_line_pos = 0;
   int line_num = 0;
   bool proceed;
   bool keep_first_word = (*xflow == 0); // if we are mid-line, don't keep the first word on this line

   /* For every "hard" line separated by a newline character... */
   hard_line = ustr_split_next(ustr, &hard_line_info, &hard_line_pos,
      linebreak);
   while (hard_line) {
      /* For every "soft" line in the "hard" line... */
      soft_line_pos = 0;
      float effective_max_width = max_width - *xflow;
      // TODO: edge cases. xflow < 0. xflow > max_width...
      soft_line =
         get_next_soft_line(hard_line, &soft_line_info, &soft_line_pos, font,
            effective_max_width, keep_first_word);
      /* No soft line here because it's an empty hard line. */
      if (!soft_line) {
         /* Call the callback with empty string to indicate an empty line. */
         proceed = cb(line_num, *xflow, *yflow, al_ustr_empty_string(), extra);
         if (!proceed) return;
         line_num++;
         *xflow = 0;
         *yflow += line_height;
      }
      while(soft_line) {
         keep_first_word = false;
         float effective_max_width = max_width - *xflow;
         /* Call the callback on the next soft line. */
         proceed = cb(line_num, *xflow, *yflow, soft_line, extra);
         *xflow += al_get_ustr_width(font, soft_line); // TODO: avoid duplicate calculation
         if (!proceed) return;
         soft_line = get_next_soft_line(hard_line, &soft_line_info,
            &soft_line_pos, font, max_width, false);
         if (soft_line) {
            line_num++;
            *xflow = 0;
            *yflow += line_height;
         }
      }
      hard_line = ustr_split_next(ustr, &hard_line_info, &hard_line_pos,
         linebreak);
   }
}

/* Helper struct for al_draw_multiline_ustr. */
typedef struct DRAW_MULTILINE_USTR_EXTRA {
   const ALLEGRO_FONT *font;
   ALLEGRO_COLOR color;
   float x;
   float y;
   float line_height;
   int flags;
} DRAW_MULTILINE_USTR_EXTRA;

/* The function draw_multiline_ustr_cb is the helper callback
 * that implements the actual drawing for al_draw_multiline_ustr.
 */
static bool draw_multiline_ustr_cb(int line_num, float xflow, float yflow, const ALLEGRO_USTR *line,
   void *extra) {
   DRAW_MULTILINE_USTR_EXTRA *s = (DRAW_MULTILINE_USTR_EXTRA *)extra;
   
   float x = s->x + xflow; //TODO: take into account alignment flags...
   float y = s->y + yflow;
   
   // TODO: line_num no longer needed, remove from callback?

   al_draw_ustr(s->font, s->color, x, y, s->flags, line);
   return true;
}

/* Function: al_draw_multiline_ustr
 */
void draw_multiline_ustr(const ALLEGRO_FONT *font,
     ALLEGRO_COLOR color, float x, float y, float *xflow, float *yflow, float max_width, float line_height,
     int flags, const ALLEGRO_USTR *ustr)
{
   DRAW_MULTILINE_USTR_EXTRA extra;
   ASSERT(font);
   ASSERT(ustr);

   extra.font = font;
   extra.color = color;
   extra.x = x;
   extra.y = y;
   if (line_height < 1) {
      extra.line_height =  al_get_font_line_height(font);
   }
   else {
      extra.line_height = line_height;
   }
   extra.flags = flags;

   do_multiline_ustr(font, xflow, yflow, extra.line_height, max_width, ustr, draw_multiline_ustr_cb, &extra);
}

/* Function: al_draw_multiline_text
 */
void draw_multiline_text(const ALLEGRO_FONT *font,
     ALLEGRO_COLOR color, float x, float y, float *xflow, float *yflow, float max_width, float line_height,
     int flags, const char *text)
{
   ALLEGRO_USTR_INFO info;
   ASSERT(font);
   ASSERT(text);

   draw_multiline_ustr(font, color, x, y, xflow, yflow, max_width, line_height, flags,
      al_ref_cstr(&info, text));
}
