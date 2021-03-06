/** @file number_of_detections_vaccination_trigger.h
 *
 * @author nharve01@uoguelph.ca<br>
 *   School of Computer Science, University of Guelph<br>
 *   Guelph, ON N1G 2W1<br>
 *   CANADA
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef NUMBER_OF_DETECTIONS_VACCINATION_TRIGGER_H
#define NUMBER_OF_DETECTIONS_VACCINATION_TRIGGER_H

adsm_module_t *number_of_detections_vaccination_trigger_new (sqlite3 *,
                                                             UNT_unit_list_t *,
                                                             projPJ,
                                                             ZON_zone_list_t *,
                                                             gpointer user_data,
                                                             GError **);
GSList *number_of_detections_vaccination_trigger_factory (sqlite3 *,
                                                          UNT_unit_list_t *,
                                                          projPJ,
                                                          ZON_zone_list_t *,
                                                          GError **);

#endif
