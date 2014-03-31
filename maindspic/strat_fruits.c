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
 *  Javier Balias Santos <javier@arc-robots.org> and Silvia Santano and Miguel Ortiz
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
#include "strat_avoid.h"
#include "strat_utils.h"
#include "sensor.h"
#include "actuator.h"
#include "beacon.h"
#include "cmdline.h"


#define ERROUT(e) do {\
		err = e;			 \
		goto end;		 \
	} while(0)


/* harvest fruits from trees */
uint8_t strat_harvest_fruits(int16_t x, int16_t y)
{
#define DEBUG_STRAT_HARVEST_FRUITS
#ifdef DEBUG_STRAT_HARVEST_FRUITS 
#define wait_press_key() state_debug_wait_key_pressed();
	strat_infos.debug_step = 1;
#else
#define wait_press_key()
#endif

   uint8_t err = 0;
	uint16_t old_spdd, old_spda, temp_spdd, temp_spda;
   int16_t d, clean_floor_a_rel;
	uint8_t stick_type;

	/* save speed */
	strat_get_speed(&old_spdd, &old_spda);
   strat_limit_speed_disable();
	strat_set_speed(SPEED_DIST_SLOW,SPEED_ANGLE_SLOW);
   

	/* depending on tree type */

	/* tree 1 */
	if (position_get_x_s16(&mainboard.pos) < TREE_2_X) {
		stick_type = I2C_STICK_TYPE_RIGHT;
		clean_floor_a_rel = 180; 
	}
	/* tree 4 */
	else 	if (position_get_x_s16(&mainboard.pos) > TREE_3_X) {
		stick_type = I2C_STICK_TYPE_LEFT;
		clean_floor_a_rel = -180; 
	}

	/* tree 2 */
	else if (position_get_x_s16(&mainboard.pos) < CENTER_X
				&& position_get_y_s16(&mainboard.pos) > TREE_1_Y) {
		stick_type = I2C_STICK_TYPE_LEFT;
		clean_floor_a_rel = -180; 
	}
	/* tree 3 */	
	else { /* if (position_get_x_s16(&mainboard.pos) > CENTER_X
				&& position_get_y_s16(&mainboard.pos) > TREE_4_Y) */
		stick_type = I2C_STICK_TYPE_RIGHT;
		clean_floor_a_rel = 180; 
	}


	/* turn in front of tree with stick deployed */
	i2c_slavedspic_mode_stick ( stick_type,
 										 I2C_STICK_MODE_CLEAN_FLOOR, 0);

	trajectory_turnto_xy (&mainboard.traj, x, y);
	err = wait_traj_end (TRAJ_FLAGS_SMALL_DIST);
	if (!TRAJ_SUCCESS(err))
		ERROUT(err);

	i2c_slavedspic_wait_ready();

	wait_press_key();

	/* clean floor */
	trajectory_a_rel (&mainboard.traj, clean_floor_a_rel);
	err = wait_traj_end (TRAJ_FLAGS_SMALL_DIST|END_NEAR);
	if (!TRAJ_SUCCESS(err))
			ERROUT(err);

	i2c_slavedspic_mode_stick ( stick_type,
 										 I2C_STICK_MODE_HIDE, 0);

	err = wait_traj_end (TRAJ_FLAGS_SMALL_DIST);
	if (!TRAJ_SUCCESS(err))
			ERROUT(err);

	wait_press_key();

	/* go to hug the tree ready for harvesting */
#define HARVEST_TREE_D_NEAR		(310)
#define HARVEST_TREE_SPEED_DIST	(1000)
#define HARVEST_TREE_D_CLOSE	 	(130)
#define HARVEST_TREE_D_FAR			(500-160)

	i2c_slavedspic_mode_harvest_fruits(I2C_SLAVEDSPIC_MODE_HARVEST_FRUITS_READY);
	i2c_slavedspic_wait_ready();

	d = distance_from_robot(x,y);
	trajectory_d_rel(&mainboard.traj, d - HARVEST_TREE_D_NEAR);
	err = wait_traj_end(TRAJ_FLAGS_SMALL_DIST);
   if (!TRAJ_SUCCESS(err))
	   ERROUT(err);

	strat_get_speed (&temp_spdd, &temp_spda);
	strat_set_speed (HARVEST_TREE_SPEED_DIST, temp_spda);

	/* should end blocking */
	trajectory_d_rel(&mainboard.traj, d);
	wait_traj_end(TRAJ_FLAGS_SMALL_DIST);
	strat_set_speed( temp_spdd, temp_spda);

	/* should stay very close to tree */
	if (distance_from_robot(x,y) > HARVEST_TREE_D_CLOSE)
		goto end_harvesting;

	wait_press_key();

	/* pick up the fruits and go backward */
	i2c_slavedspic_mode_harvest_fruits(I2C_SLAVEDSPIC_MODE_HARVEST_FRUITS_DO);
	i2c_slavedspic_wait_ready();

end_harvesting:

	/* check if opponent is behind */
#ifdef TODO	
	if (opponent_is_infront()) {

		strat_set_speed (SPEED_DIST_SLOW, SPEED_ANGLE_VERY_SLOW);

		/* can be ends blocking */
		trajectory_d_rel(&mainboard.traj, -HARVEST_TREE_D_FAR);
		err = wait_traj_end(TRAJ_FLAGS_SMALL_DIST);

		if (distance_from_robot(x,y) < HARVEST_TREE_D_NEAR) {
			trajectory_d_rel(&mainboard.traj, );
			
		}

		/* try to escape from */
		if (TRAJ_BLOCKING(err)) {

		}


	}
	else 
#endif 
	{
		trajectory_d_rel(&mainboard.traj, -HARVEST_TREE_D_FAR);
		err = wait_traj_end(TRAJ_FLAGS_SMALL_DIST);
		if (!TRAJ_SUCCESS(err))
			ERROUT(err);
	}

	/* hide tools */

end:
	i2c_slavedspic_mode_harvest_fruits(I2C_SLAVEDSPIC_MODE_HARVEST_FRUITS_DO);
	strat_set_speed(old_spdd, old_spda);	
   strat_limit_speed_enable();
   return err;
}












