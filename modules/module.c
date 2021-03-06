/** @file module.c
 * Implementations of default methods for simulator modules.
 *
 * @author Neil Harvey <nharve01@uoguelph.ca><br>
 *   School of Computer Science, University of Guelph<br>
 *   Guelph, ON N1G 2W1<br>
 *   CANADA
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#if HAVE_CONFIG_H
#  include "config.h"
#endif

#include "module.h"



/**
 * Copies a 0-terminated list of event types into a newly-allocated GArray.
 *
 * @param event_types a NULL-terminated list of event types.
 * @return a GArray containing the event types.
 */
GArray *
adsm_setup_events_listened_for (EVT_event_type_t *event_types)
{
  GArray *arr;
  EVT_event_type_t *iter;

  arr = g_array_new (/* zero_terminated = */ FALSE,
                     /* clear = */ FALSE,
                     sizeof (EVT_event_type_t));
  for (iter = event_types; *iter != 0; iter++)
    {
      g_array_append_val (arr, *iter);
    }

  return arr;
} 



/**
 * Reports whether a module is listening for a given event type.
 *
 * @param self the module.
 * @param event_type an event type.
 * @return TRUE if the module is listening for the event type.
 */
gboolean
adsm_model_is_listening_for (struct adsm_module_t_ *self, EVT_event_type_t event_type)
{
  guint n, i;

  n = self->events_listened_for->len;
  for (i = 0; i < n; i++)
    if (g_array_index (self->events_listened_for, EVT_event_type_t, i) == event_type)
      return TRUE;
  return FALSE;
}



/**
 * Returns a text representation of a module, containing just the module name.
 *
 * @param self the module.
 * @return a string. Must be freed using g_free().
 */
char *
adsm_module_to_string_default (struct adsm_module_t_ *self)
{
  GString *s;
  char *chararray;

  s = g_string_new (NULL);
  g_string_sprintf (s, "<%s>", self->name);

  /* don't return the wrapper object */
  chararray = s->str;
  g_string_free (s, FALSE);
  return chararray;
}



/**
 * Prints a module to a stream.
 *
 * @param stream a stream to write to.
 * @param self the module.
 * @return the number of characters printed (not including the trailing '\\0').
 */
int
adsm_model_fprintf (FILE * stream, struct adsm_module_t_ *self)
{
  char *s;
  int nchars_written;

  s = self->to_string (self);
  nchars_written = fprintf (stream, "%s", s);
  free (s);
  return nchars_written;
}



/**
 * Prints a module.
 *
 * @param self the module.
 * @return the number of characters printed (not including the trailing '\\0').
 */
int
adsm_model_printf (struct adsm_module_t_ *self)
{
  return adsm_model_fprintf (stdout, self);
}



/**
 * Returns TRUE. Useful for filling in the function pointers for yes/no
 * questions that a module can answer, such as whether the module has pending
 * actions.
 *
 * @param self the module.
 * @return TRUE.
 */
gboolean
adsm_model_answer_yes (struct adsm_module_t_ * self)
{
  return TRUE;
}



/**
 * Returns FALSE. Useful for filling in the function pointers for yes/no
 * questions that a module can answer, such as whether the module has pending
 * actions.
 *
 * @param self the module.
 * @return FALSE.
 */
gboolean
adsm_model_answer_no (struct adsm_module_t_ * self)
{
  return FALSE;
}



/**
 * Creates a Declaration Of Outputs event containing all of the modules' output
 * variables, and submits that event to an event queue.
 *
 * @param self a module
 * @param queue an event queue
 */
void
adsm_declare_outputs (struct adsm_module_t_ * self, EVT_event_queue_t * queue)
{
  GPtrArray *output_list;
  unsigned int i;

  if (self->outputs->len > 0)
    {
      output_list = g_ptr_array_new ();
      for (i = 0; i < self->outputs->len; i++)
        {
          g_ptr_array_add (output_list, g_ptr_array_index (self->outputs, i));
        }
      EVT_event_enqueue (queue, EVT_new_declaration_of_outputs_event (output_list));
      /* Note that we don't clean up the GPtrArray.  It will be freed along
       * with the declaration event after all interested modules have processed
       * the event. */
    }

  return;
}



/* end of file module.c */
