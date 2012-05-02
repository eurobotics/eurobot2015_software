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
 *  Javier Bali�as Santos <javier@arc-robots.org>
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
#include <time.h>

#include <pid.h>
#include <quadramp.h>
#include <control_system_manager.h>
#include <trajectory_manager.h>
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



uint8_t strat_begin(void)
{   
   uint8_t err;
	uint16_t old_spdd, old_spda;
   int16_t d;
   uint32_t old_var_2nd_ord_pos, old_var_2nd_ord_neg;

   #define X_CORNER_1        628
   #define Y_CORNER_1       1353
   #define X_CORNER_2       900
   #define Y_CORNER_2       Y_CORNER_1+ROBOT_WIDTH/2+50
   #define LONG_DISTANCE    2500
   #define Y_BEGIN_CURVE    1050
   #define X_END_CURVE      1100

   DEBUG(E_USER_STRAT, "Start");
	/* save speed */
	strat_get_speed(&old_spdd, &old_spda);
   strat_limit_speed_disable();
	strat_set_speed(SPEED_DIST_FAST,SPEED_ANGLE_FAST);

   /* save quadramp values */
   old_var_2nd_ord_pos = mainboard.angle.qr.var_2nd_ord_pos;
   old_var_2nd_ord_neg = mainboard.angle.qr.var_2nd_ord_neg;

   /* fingers in hold mode and put lift down */
   i2c_slavedspic_mode_fingers(I2C_FINGERS_TYPE_FLOOR, I2C_FINGERS_MODE_HOLD,0);
   i2c_slavedspic_mode_fingers(I2C_FINGERS_TYPE_TOTEM, I2C_FINGERS_MODE_HOLD,0);

   /* get out of home */
   d= distance_from_robot(COLOR_X(X_CORNER_1), position_get_y_s16(&mainboard.pos));
   trajectory_d_rel(&mainboard.traj, d);
	err = wait_traj_end(TRAJ_FLAGS_NO_NEAR);
	if (!TRAJ_SUCCESS(err))
			ERROUT(err);

   i2c_slavedspic_wait_ready();
   i2c_slavedspic_mode_lift_height(44);

   /* go to point close to the bottles */
#ifdef VERSION1

   if(strat_infos.conf.flags & PICKUP_COINS_GROUP)
      trajectory_goto_forward_xy_abs(&mainboard.traj,COLOR_X(X_CORNER_1), Y_CORNER_1);
   else
      trajectory_goto_backward_xy_abs(&mainboard.traj,COLOR_X(X_CORNER_1), Y_CORNER_1);

	err = wait_traj_end(TRAJ_FLAGS_STD);
	if (!TRAJ_SUCCESS(err))
			ERROUT(err);
   
   trajectory_goto_xy_abs(&mainboard.traj,COLOR_X(X_CORNER_2),Y_CORNER_2);
	err = wait_traj_end(TRAJ_FLAGS_STD);
	if (!TRAJ_SUCCESS(err))
			ERROUT(err);   
 
   /* We decide to pickup the coins or the goldbar depending on strategy */
   trajectory_goto_xy_abs(&mainboard.traj,COLOR_X(1218),Y_CORNER_2);
	err = wait_traj_end(TRAJ_FLAGS_NO_NEAR);
	if (!TRAJ_SUCCESS(err))
			ERROUT(err);
#endif

   if(strat_infos.conf.flags & PICKUP_COINS_GROUP)
      trajectory_a_abs(&mainboard.traj, 90);
   else
      trajectory_a_abs(&mainboard.traj, -90);
	err = wait_traj_end(TRAJ_FLAGS_NO_NEAR);
	if (!TRAJ_SUCCESS(err))
			ERROUT(err);   


   /* modify speed and quadramp */
   quadramp_set_2nd_order_vars(&mainboard.angle.qr, 20, 20);
	strat_set_speed(SPEED_DIST_FAST,2000);


   if(strat_infos.conf.flags & PICKUP_COINS_GROUP)
      trajectory_d_rel(&mainboard.traj, LONG_DISTANCE);
   else
      trajectory_d_rel(&mainboard.traj, -LONG_DISTANCE);
   
   err=WAIT_COND_OR_TRAJ_END(y_is_more_than(Y_BEGIN_CURVE), TRAJ_FLAGS_STD);
   if(err) 
   	if (!TRAJ_SUCCESS(err))
			ERROUT(err);   


   if(strat_infos.conf.flags & PICKUP_COINS_GROUP)
      trajectory_only_a_abs(&mainboard.traj, COLOR_A_ABS(0));
   else
      trajectory_only_a_abs(&mainboard.traj, COLOR_A_ABS(180));

   err=WAIT_COND_OR_TRAJ_END(x_is_more_than(COLOR_X(X_END_CURVE)), TRAJ_FLAGS_STD);
   if(err) 
   	if (!TRAJ_SUCCESS(err))
			ERROUT(err);   

   trajectory_goto_xy_abs(&mainboard.traj, COLOR_X(1500), position_get_y_s16(&mainboard.pos));
	err = wait_traj_end(TRAJ_FLAGS_NO_NEAR);
	if (!TRAJ_SUCCESS(err))
			ERROUT(err);   

   end:
	strat_set_speed(old_spdd, old_spda);
   strat_limit_speed_enable();

   /* restore quadramp values */
   quadramp_set_2nd_order_vars(&mainboard.angle.qr,old_var_2nd_ord_pos,old_var_2nd_ord_neg);
   return err;
}

