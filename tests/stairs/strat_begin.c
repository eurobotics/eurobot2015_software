/*  
 *  Copyright Robotics Association of Coslada, Eurobotics Engineering (2011)
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Revision : $Id$
 *
 *  Javier Balias Santos <javier@arc-robots.org> and Silvia Santano
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <aversive/pgmspace.h>
#include <aversive/queue.h>
#include <aversive/wait.h>
#include <aversive/error.h>

#include <uart.h>
#include <dac_mc.h>
#include <pwm_servo.h>
#include <clock_time.h>

#include <pid.h>
#include <quadramp.h>
#include <control_system_manager.h>
#include <trajectory_manager.h>
#include <trajectory_manager_utils.h>
//#include <trajectory_manager_core.h>
#include <vect_base.h>
#include <lines.h>
#include <polygon.h>
#include <obstacle_avoidance.h>
#include <blocking_detection_manager.h>
#include <robot_system.h>
#include <position_manager.h>


#include <rdline.h>
#include <parse.h>

#include "../common/i2c_commands.h"
#include "i2c_protocol.h"
#include "main.h"
#include "strat.h"
#include "strat_base.h"
#include "../maindspic/strat_avoid.h"
#include "../maindspic/strat_utils.h"
#include "sensor.h"
#include "actuator.h"
#include "beacon.h"
#include "cmdline.h"


#define ERROUT(e) do {\
		err = e;			 \
		goto end;		 \
	} while(0)



uint8_t strat_begin(void)
{  
	uint8_t err;
	uint8_t i=0;
	#define ZONES_SEQUENCE_LENGTH 4

    int16_t d;
	uint8_t zones_sequence[ZONES_SEQUENCE_LENGTH];
	
	for(i=0; i<ZONES_SEQUENCE_LENGTH; i++)
	{
		/* goto zone */
		//printf_P(PSTR("Going to zone %s.\r\n"),numzone2name[zones_sequence[i]]);
		strat_dump_infos(__FUNCTION__);
		strat_infos.current_zone=-1;
		strat_infos.goto_zone=zones_sequence[i];

		strat_goto_zone (zones_sequence[i]);
		err = wait_traj_end(TRAJ_FLAGS_STD);
		strat_infos.last_zone=strat_infos.current_zone;
		
		if (!TRAJ_SUCCESS(err)) {
			strat_infos.current_zone=-1;
			//printf_P(PSTR("Can't reach zone %s. err=%s\r\n"), numzone2name[zones_sequence[i]],get_err(err));
		}
		else{
			strat_infos.current_zone=zones_sequence[i];
		}

		strat_infos.goto_zone=-1;


		/* work on zone */
		strat_dump_infos(__FUNCTION__);
		err = strat_work_on_zone(zones_sequence[i]);
		if (!TRAJ_SUCCESS(err)) {
			//printf_P(PSTR("Work on zone %s fails.\r\n"), numzone2name[zones_sequence[i]]);
		}

		/* mark the zone as checked */
		strat_infos.zones[zones_sequence[i]].flags |= ZONE_CHECKED;
		//printf_P(PSTR("finished zone %d.\r\n"),i);
	}
end:
	return err;
}

#define DEBUG_BEGIN
#ifdef DEBUG_BEGIN 
#define wait_press_key() state_debug_wait_key_pressed();
#else
#define wait_press_key()
#endif


