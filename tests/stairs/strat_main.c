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
 *  Javier Bali?as Santos <javier@arc-robots.org> and Silvia Santano 
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
#include <pwm_mc.h>
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
#include <scheduler.h>


#include <rdline.h>
#include <parse.h>

#include "../common/i2c_commands.h"
//#include "../common/bt_commands.h"

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

/* Add here the main strategic, the inteligence of robot */


/* auto possition depending on color */
void strat_auto_position (void)
{
#define AUTOPOS_SPEED_FAST 	1000
#define BASKET_WIDTH		300

	uint8_t err;
	uint16_t old_spdd, old_spda;

	/* save & set speeds */
	interrupt_traj_reset();
	strat_get_speed(&old_spdd, &old_spda);
	strat_set_speed(AUTOPOS_SPEED_FAST, AUTOPOS_SPEED_FAST);

	/* goto blocking to y axis */
	trajectory_d_rel(&mainboard.traj, -200);
	err = wait_traj_end(END_INTR|END_TRAJ|END_BLOCKING);
	if (err == END_INTR)
		goto intr;
	wait_ms(100);

	/* set y */
	strat_reset_pos(DO_NOT_SET_POS, BASKET_WIDTH + ROBOT_CENTER_TO_BACK, 90);

	/* prepare to x axis */
	trajectory_d_rel(&mainboard.traj, 60);
	err = wait_traj_end(END_INTR|END_TRAJ);
	if (err == END_INTR)
		goto intr;

	trajectory_a_rel(&mainboard.traj, COLOR_A_REL(-90));
	err = wait_traj_end(END_INTR|END_TRAJ);
	if (err == END_INTR)
		goto intr;


	/* goto blocking to x axis */
	trajectory_d_rel(&mainboard.traj, -700);
	err = wait_traj_end(END_INTR|END_TRAJ|END_BLOCKING);
	if (err == END_INTR)
		goto intr;
	wait_ms(100);
	
	/* set x and angle */
	strat_reset_pos(COLOR_X(ROBOT_CENTER_TO_BACK), DO_NOT_SET_POS, COLOR_A_ABS(0));
	
	/* goto start position */
	trajectory_d_rel(&mainboard.traj, 150);
	err = wait_traj_end(END_INTR|END_TRAJ);
	if (err == END_INTR)
		goto intr;
	wait_ms(100);
	
	/* restore speeds */	
	strat_set_speed(old_spdd, old_spda);
	return;

intr:
	strat_hardstop();
	strat_set_speed(old_spdd, old_spda);
}


#if 0
void strat_initial_move(void)
{
#define BEGIN_LINE_Y 	450
#define BEGIN_FRESCO_X	1295
	static uint8_t state = 0;
	uint8_t err = 0;
	
	
	while(1)
	{
		switch(state)
		{
			/* go in front of fresco */
			case 0:
				//printf_P("init case 0\n");
				trajectory_goto_xy_abs (&mainboard.traj,  COLOR_X(BEGIN_FRESCO_X), BEGIN_LINE_Y);
				state++;
				break;
			
			case 1:
				//printf_P("init case 1 - x: %d y: %d - opp %d\n", abs(position_get_x_double(&mainboard.pos)-COLOR_X(BEGIN_FRESCO_X)),abs(position_get_y_double(&mainboard.pos)-BEGIN_LINE_Y) , (opponent1_is_infront() || opponent2_is_infront()));
				if(opponent1_is_infront()==0 && opponent2_is_infront()==0)
				{
					//err = test_traj_end(TRAJ_FLAGS_NO_NEAR);

					if ((abs(position_get_x_double(&mainboard.pos)-COLOR_X(BEGIN_FRESCO_X))<30) && (abs(position_get_y_double(&mainboard.pos)-BEGIN_LINE_Y)<30))
					{
						state ++;
					}
				}
				else
				{
					if ((abs(position_get_x_double(&mainboard.pos)-COLOR_X(BEGIN_FRESCO_X))<30) && (abs(position_get_y_double(&mainboard.pos)-BEGIN_LINE_Y)<30))
					{
						state ++;
						break;
					}
					strat_hardstop();
					state=0;
				}	
				break;
				
			case 2:
				trajectory_a_abs (&mainboard.traj, 90);
				err = wait_traj_end(TRAJ_FLAGS_NO_NEAR);
				if (!TRAJ_SUCCESS(err))
						break;
				return;
				break;
			default:
				break;
		}
	}
}
#endif
