/** @file full_table_writer.c
 * Writes out a table of values covering many aspects of the simulation, in
 * comma-separated values (csv) format.
 *
 * @author Neil Harvey <nharve01@uoguelph.ca><br>
 *   School of Computer Science, University of Guelph<br>
 *   Guelph, ON N1G 2W1<br>
 *   CANADA
 *
 * Copyright &copy; University of Guelph, 2009
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#if HAVE_CONFIG_H
#  include "config.h"
#endif

/* To avoid name clashes when multiple modules have the same interface. */
#define new full_table_writer_new
#define run full_table_writer_run
#define local_free full_table_writer_free
#define handle_declaration_of_outputs_event full_table_writer_handle_declaration_of_outputs_event
#define handle_before_each_simulation_event full_table_writer_handle_before_each_simulation_event
#define handle_new_day_event full_table_writer_handle_new_day_event
#define handle_unit_state_change_event full_table_writer_handle_unit_state_change_event
#define handle_request_for_zone_focus_event full_table_writer_handle_request_for_zone_focus_event
#define handle_vaccination_event full_table_writer_handle_vaccination_event
#define handle_destruction_event full_table_writer_handle_destruction_event
#define handle_end_of_day2_event full_table_writer_handle_end_of_day2_event

#include "module.h"
#include "module_util.h"

#include <unistd.h>

#if STDC_HEADERS
#  include <string.h>
#endif

#if HAVE_MATH_H
#  include <math.h>
#endif

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#include "full_table_writer.h"

#if !HAVE_ROUND && HAVE_RINT
#  define round rint
#endif

/* Temporary fix -- "round" and "rint" are in the math library on Red Hat 7.3,
 * but they're #defined so AC_CHECK_FUNCS doesn't find them. */
double round (double x);

/** This must match an element name in the DTD. */
#define MODEL_NAME "full-table-writer"



/* A structure used to store how many times certain events happened to a unit
 * during the simulation. */
typedef struct
{
  guint was_infected;
  guint was_zone_focus;
  guint was_vaccinated;
  guint was_destroyed;
}
unit_stats_t;

/**
 * Returns a new unit_stats_t structure, initialized to zeroes.
 */
unit_stats_t *
new_unit_stats (void)
{
  unit_stats_t *self;
  self = g_new (unit_stats_t, 1);
  self->was_infected = self->was_zone_focus = self->was_vaccinated = self->was_destroyed = 0;
  return self;
}



/* Specialized information for this model. */
typedef struct
{
  GIOChannel *channel; /* The output channel. */
  int run_number;
  gboolean printed_header;
  GString *buf;
  gboolean write_unit_stats;
  GHashTable *unit_stats; /* Key = unit (UNT_unit_t *), value = pointer to a
    a unit_stats_t struct, storing what happened to the unit during the
    iteration. */
}
local_data_t;



/**
 * Responds to an "declaration of outputs" event by adding the output
 * variable(s) to its list of outputs to report.
 *
 * @param self the model.
 * @param event a declaration of outputs event.
 */
void
handle_declaration_of_outputs_event (struct adsm_module_t_ * self,
                                     EVT_declaration_of_outputs_event_t *event)
{
  unsigned int n, i;

#if DEBUG
  g_debug ("----- ENTER handle_declaration_of_outputs_event (%s)", MODEL_NAME);
#endif

  n = event->outputs->len;
  for (i = 0; i < n; i++)
    g_ptr_array_add (self->outputs, g_ptr_array_index (event->outputs, i));
#if DEBUG
  g_debug ("%s now has %u output variables", MODEL_NAME, self->outputs->len);
#endif

#if DEBUG
  g_debug ("----- EXIT handle_declaration_of_outputs_event (%s)", MODEL_NAME);
#endif
}



/**
 * Before each simulation, this module increments its "run number" and deletes
 * any records about units being infected, vaccinated, etc. left over from a
 * previous iteration.
 *
 * @event a Before Each Simulation event.
 * @param self the model.
 */
void
handle_before_each_simulation_event (struct adsm_module_t_ * self,
                                     EVT_before_each_simulation_event_t * event)
{
  local_data_t *local_data;

#if DEBUG
  g_debug ("----- ENTER handle_before_each_simulation_event (%s)", MODEL_NAME);
#endif

  local_data = (local_data_t *) (self->model_data);
  local_data->run_number = event->iteration_number;
  g_hash_table_remove_all (local_data->unit_stats);

#if DEBUG
  g_debug ("----- EXIT handle_before_each_simulation_event (%s)", MODEL_NAME);
#endif
}



/**
 * On the first "new day" event of a set of simulations, we write the table
 * header.  It would be nicer to do this before any simulations begin, but we
 * need to be sure that the output variables have been initialized with all
 * causes of infection, reasons for destruction, etc. before the table header
 * is written.
 *
 * @param self the model.
 * @param event a new day event.
 */
void
handle_new_day_event (struct adsm_module_t_ * self, EVT_new_day_event_t * event)
{
  local_data_t *local_data;
  unsigned int i;
  RPT_reporting_t *reporting;
  char *camel;
  GError *error = NULL;

#if DEBUG
  g_debug ("----- ENTER handle_new_day_event (%s)", MODEL_NAME);
#endif

  local_data = (local_data_t *) (self->model_data);

  /* Print the table header, if we haven't already. */
  if (!local_data->printed_header)
    {
      /* The first two fields are run and day. */
      g_string_printf (local_data->buf, "Run,Day");

      /* Output the other variables in the order they were created in the
       * new() function. */
      for (i = 0; i < self->outputs->len; i++)
        {
          reporting = (RPT_reporting_t *) g_ptr_array_index (self->outputs, i);
          camel = camelcase (reporting->name, /* capitalize first = */ FALSE); 
          g_string_append_printf (local_data->buf, ",%s", camel);
          g_free (camel);
        }
      g_string_append_c (local_data->buf, '\n');
      g_io_channel_write_chars (local_data->channel, local_data->buf->str, 
                                -1 /* assume null-terminated string */,
                                NULL, &error);
      g_io_channel_flush (local_data->channel, &error);

      local_data->printed_header = TRUE;
    }  

#if DEBUG
  g_debug ("----- EXIT handle_new_day_event (%s)", MODEL_NAME);
#endif
  return;
}



/**
 * Responds to a Unit State Change to an infected state by recording that the
 * unit was infected during this iteration.
 *
 * @param self this module.
 * @param event a Unit State Change event.
 */
void
handle_unit_state_change_event (struct adsm_module_t_ * self,
                                EVT_unit_state_change_event_t * event)
{
  local_data_t *local_data;
  unit_stats_t *unit_stats;

  #if DEBUG
    g_debug ("----- ENTER handle_unit_state_change_event (%s)", MODEL_NAME);
  #endif

  /* We are only interested in state changes to an infected state. */
  if (event->old_state == Susceptible
      && (event->new_state == Latent
          || event->new_state == InfectiousSubclinical
          || event->new_state == InfectiousClinical))
    {
      local_data = (local_data_t *) (self->model_data);
      unit_stats = g_hash_table_lookup (local_data->unit_stats, event->unit);
      if (unit_stats == NULL)
        {
          unit_stats = new_unit_stats();
          g_hash_table_insert (local_data->unit_stats, event->unit, unit_stats);
        }
      unit_stats->was_infected++;
    }

  #if DEBUG
    g_debug ("----- EXIT handle_unit_state_change_event (%s)", MODEL_NAME);
  #endif

  return;
}



/**
 * Responds to a RequestForZoneFocus event by recording that a zone was created
 * around the unit during this iteration.
 *
 * @param self this module.
 * @param event a Request For Zone Focus event.
 */
void
handle_request_for_zone_focus_event (struct adsm_module_t_ * self,
                                     EVT_request_for_zone_focus_event_t * event)
{
  local_data_t *local_data;
  unit_stats_t *unit_stats;

  #if DEBUG
    g_debug ("----- ENTER handle_request_for_zone_focus_event (%s)", MODEL_NAME);
  #endif

  local_data = (local_data_t *) (self->model_data);
  unit_stats = g_hash_table_lookup (local_data->unit_stats, event->unit);
  if (unit_stats == NULL)
    {
      unit_stats = new_unit_stats();
      g_hash_table_insert (local_data->unit_stats, event->unit, unit_stats);
    }
  unit_stats->was_zone_focus++;

  #if DEBUG
    g_debug ("----- EXIT handle_request_for_zone_focus_event (%s)", MODEL_NAME);
  #endif

  return;
}



/**
 * Responds to a Vaccination event by recording that the unit was vaccinated
 * during this iteration.
 *
 * @param self this module.
 * @param event a Vaccination event.
 */
void
handle_vaccination_event (struct adsm_module_t_ * self,
                          EVT_vaccination_event_t * event)
{
  local_data_t *local_data;
  unit_stats_t *unit_stats;

  #if DEBUG
    g_debug ("----- ENTER handle_vaccination_event (%s)", MODEL_NAME);
  #endif

  local_data = (local_data_t *) (self->model_data);
  unit_stats = g_hash_table_lookup (local_data->unit_stats, event->unit);
  if (unit_stats == NULL)
    {
      unit_stats = new_unit_stats();
      g_hash_table_insert (local_data->unit_stats, event->unit, unit_stats);
    }
  unit_stats->was_vaccinated++;

  #if DEBUG
    g_debug ("----- EXIT handle_vaccination_event (%s)", MODEL_NAME);
  #endif

  return;
}



/**
 * Responds to a Destruction event by recording that the unit was destroyed
 * during this iteration.
 *
 * @param self this module.
 * @param event a Destruction event.
 */
void
handle_destruction_event (struct adsm_module_t_ * self,
                          EVT_destruction_event_t * event)
{
  local_data_t *local_data;
  unit_stats_t *unit_stats;

  #if DEBUG
    g_debug ("----- ENTER handle_destruction_event (%s)", MODEL_NAME);
  #endif

  local_data = (local_data_t *) (self->model_data);
  unit_stats = g_hash_table_lookup (local_data->unit_stats, event->unit);
  if (unit_stats == NULL)
    {
      unit_stats = new_unit_stats();
      g_hash_table_insert (local_data->unit_stats, event->unit, unit_stats);
    }
  unit_stats->was_destroyed++;

  #if DEBUG
    g_debug ("----- EXIT handle_destruction_event (%s)", MODEL_NAME);
  #endif

  return;
}



/**
 * Responds to an end of day event by writing a table row.
 *
 * @param self the model.
 * @param event an end of day event.
 */
void
handle_end_of_day2_event (struct adsm_module_t_ * self,
                          EVT_end_of_day2_event_t * event)
{
  local_data_t *local_data;
  unsigned int i;
  RPT_reporting_t *reporting;
  char *value;
  GError *error = NULL;

#if DEBUG
  g_debug ("----- ENTER handle_end_of_day2_event (%s)", MODEL_NAME);
#endif

  local_data = (local_data_t *) (self->model_data);

  /* An end-of-day event is currently used to set up the initially infected,
   * vaccinated, and destroyed units.  That will appear as "day 0", i.e.,
   * before the first day of the simulation.  Don't print a row for day 0. */
  if (event->day > 0)
    {
      /* The first two fields are run and day. */
      g_string_printf (local_data->buf, "%i,%i", local_data->run_number, event->day);

      /* Output the other variables in the order they were created in the
       * new() function. */
      for (i = 0; i < self->outputs->len; i++)
        {
          reporting = (RPT_reporting_t *) g_ptr_array_index (self->outputs, i);
          value = RPT_reporting_value_to_string (reporting, NULL);
          g_string_append_printf (local_data->buf, ",%s", value);
          g_free (value);
        } /* end of loop over output variables */
      g_string_append_c (local_data->buf, '\n');
      g_io_channel_write_chars (local_data->channel, local_data->buf->str, 
                                -1 /* assume null-terminated string */,
                                NULL, &error);
    }

#if DEBUG
  g_debug ("----- EXIT handle_end_of_day2_event (%s)", MODEL_NAME);
#endif
  return;
}



/**
 * Outputs a row for one unit in the database, saying how many times the unit
 * was infected, vaccinated, etc.
 *
 * @param key a unit
 * @param value a pointer to a unit_stats_t struct
 * @param user_data local_data
 */
static void
write_unit_stats (gpointer key, gpointer value, gpointer user_data)
{
  UNT_unit_t *unit;
  unit_stats_t *unit_stats;
  local_data_t *local_data;
  GError *error = NULL;

  unit = (UNT_unit_t *) key;
  unit_stats = (unit_stats_t *) value;
  local_data = (local_data_t *) (user_data);

  g_string_printf (local_data->buf, "%u,%u,%u,%u,%u\n",
                   unit->db_id,
                   unit_stats->was_infected,
                   unit_stats->was_zone_focus,
                   unit_stats->was_vaccinated,
                   unit_stats->was_destroyed);
  g_io_channel_write_chars (local_data->channel, local_data->buf->str, 
                            -1 /* assume null-terminated string */,
                            NULL, &error);

  return;
}



/**
 * Runs this model.
 *
 * @param self the model.
 * @param units a list of units.
 * @param zones a list of zones.
 * @param event the event that caused the model to run.
 * @param rng a random number generator.
 * @param queue for any new events the model creates.
 */
void
run (struct adsm_module_t_ *self, UNT_unit_list_t * units,
     ZON_zone_list_t * zones, EVT_event_t * event, RAN_gen_t * rng, EVT_event_queue_t * queue)
{
#if DEBUG
  g_debug ("----- ENTER run (%s)", MODEL_NAME);
#endif

  switch (event->type)
    {
    case EVT_DeclarationOfOutputs:
      handle_declaration_of_outputs_event (self, &(event->u.declaration_of_outputs));
      break;
    case EVT_BeforeEachSimulation:
      handle_before_each_simulation_event (self, &(event->u.before_each_simulation));
      break;
    case EVT_NewDay:
      handle_new_day_event (self, &(event->u.new_day));
      break;
    case EVT_UnitStateChange:
      handle_unit_state_change_event (self, &(event->u.unit_state_change));
      break;
    case EVT_RequestForZoneFocus:
      handle_request_for_zone_focus_event (self, &(event->u.request_for_zone_focus));
      break;
    case EVT_Vaccination:
      handle_vaccination_event (self, &(event->u.vaccination));
      break;
    case EVT_Destruction:
      handle_destruction_event (self, &(event->u.destruction));
      break;
    case EVT_EndOfDay2:
      handle_end_of_day2_event (self, &(event->u.end_of_day2));
      break;
    default:
      g_error
        ("%s has received a %s event, which it does not listen for.  This should never happen.  Please contact the developer.",
         MODEL_NAME, EVT_event_type_name[event->type]);
    }

#if DEBUG
  g_debug ("----- EXIT run (%s)", MODEL_NAME);
#endif
}



/**
 * Frees this model.
 *
 * @param self the model.
 */
void
local_free (struct adsm_module_t_ *self)
{
  local_data_t *local_data;
  GError *error = NULL;

#if DEBUG
  g_debug ("----- ENTER free (%s)", MODEL_NAME);
#endif

  local_data = (local_data_t *) (self->model_data);

  /* If no runs were ever done, don't bother writing the unit stats. (This can
   * happen when using the --dry-run switch.) Since the header for the numeric
   * table is printed when the first New Day event is received, we can use the
   * value of printed_header as a test of whether any runs were done. */
  if (local_data->write_unit_stats && local_data->printed_header)
    {
      /* Write the unit stats table. The free function isn't a great place to do
       * this; maybe an AfterAllSimulations event is needed. */
      g_string_printf (local_data->buf, "\nunit,infected,zone_focus,vaccinated,destroyed\n");
      g_io_channel_write_chars (local_data->channel, local_data->buf->str, 
                                -1 /* assume null-terminated string */,
                                NULL, &error);
      g_hash_table_foreach (local_data->unit_stats, write_unit_stats, local_data);
    }

  /* Flush any remaining output. */
  g_io_channel_flush (local_data->channel, &error);

  /* Free the dynamically-allocated parts. */
  g_string_free (local_data->buf, TRUE);
  g_hash_table_destroy (local_data->unit_stats);
  g_free (local_data);
  g_ptr_array_free (self->outputs, TRUE);
  g_free (self);

#if DEBUG
  g_debug ("----- EXIT free (%s)", MODEL_NAME);
#endif
}



/**
 * Returns a new full table writer.
 */
adsm_module_t *
new (sqlite3 * params, UNT_unit_list_t * units, projPJ projection,
     ZON_zone_list_t * zones, GError **error)
{
  adsm_module_t *self;
  local_data_t *local_data;
  EVT_event_type_t events_listened_for[] = {
    EVT_DeclarationOfOutputs,
    EVT_BeforeEachSimulation,
    EVT_NewDay,
    EVT_UnitStateChange,
    EVT_RequestForZoneFocus,
    EVT_Vaccination,
    EVT_Destruction,
    EVT_EndOfDay2,
    0
  };

#if DEBUG
  g_debug ("----- ENTER new (%s)", MODEL_NAME);
#endif

  self = g_new (adsm_module_t, 1);
  local_data = g_new (local_data_t, 1);

  self->name = MODEL_NAME;
  self->events_listened_for = adsm_setup_events_listened_for (events_listened_for);
  self->outputs = g_ptr_array_new ();
  self->model_data = local_data;
  self->run = run;
  self->is_listening_for = adsm_model_is_listening_for;
  self->has_pending_actions = adsm_model_answer_no;
  self->has_pending_infections = adsm_model_answer_no;
  self->to_string = adsm_module_to_string_default;
  self->printf = adsm_model_printf;
  self->fprintf = adsm_model_fprintf;
  self->free = local_free;

  /* This module maintains no output variables of its own.  Before any
   * simulations begin, we will gather all the output variables available from
   * other modules. */

  #ifdef G_OS_UNIX
    local_data->channel = g_io_channel_unix_new (STDOUT_FILENO);
  #endif
  #ifdef G_OS_WIN32
    local_data->channel = g_io_channel_win32_new_fd (STDOUT_FILENO);
  #endif
  local_data->printed_header = FALSE;
  local_data->buf = g_string_new (NULL);
  local_data->unit_stats =
    g_hash_table_new_full (g_direct_hash, g_direct_equal,
                           /* key_destroy_func = */ NULL,
                           /* value_destroy_func = */ g_free);

  /* Check if the flag to write unit stats is set. */
  local_data->write_unit_stats = PAR_get_boolean (params, "SELECT (save_iteration_outputs_for_units=1) FROM ScenarioCreator_outputsettings");

#if DEBUG
  g_debug ("----- EXIT new (%s)", MODEL_NAME);
#endif

  return self;
}

/* end of file full_table_writer.c */
