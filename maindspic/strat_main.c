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
 *  Javier Bali�as Santos <javier@arc-robots.org> and Silvia Santano
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
#include "bt_protocol.h"


#define ERROUT(e) do {\
		err = e;			 \
		goto end;		 \
	} while(0)


/* Add here the main strategy, the intelligence of robot */

/* debug step to step */
void state_debug_wait_key_pressed (void)
{
	if (!strat_infos.debug_step)
		return;

	DEBUG(E_USER_STRAT,"press a key");
	while(!cmdline_keypressed());
}

/* debug strat step to step */
void strat_debug_wait_key_pressed (uint8_t robot)
{
	if (!strat_infos.debug_step)
		return;

	DEBUG(E_USER_STRAT,"%s, press a key",
			robot==MAIN_ROBOT? "R1":"R2");

	while(!cmdline_keypressed());
}

/* debug strat step to step */
uint8_t strat_debug_is_key_pressed (uint8_t robot)
{
	if (!strat_infos.debug_step)
		return 1;

	DEBUG(E_USER_STRAT,"%s, press a key",
			robot==MAIN_ROBOT? "R1":"R2");

	return cmdline_keypressed();
}


static uint8_t strat_secondary_robot_on = 0;

void strat_secondary_robot_enable (void)
{
	uint8_t flags;

	IRQ_LOCK(flags);
	strat_secondary_robot_on = 1;
	IRQ_UNLOCK(flags);
}

void strat_secondary_robot_disable (void)
{
	uint8_t flags;

	IRQ_LOCK(flags);
	strat_secondary_robot_on = 0;
	IRQ_UNLOCK(flags);
}

uint8_t strat_secondary_robot_is_enabled (void)
{
	return strat_secondary_robot_on;
}


#ifdef old_version
/*
 * Check if the zone is available.
 * @Param zone_num checked zone
 * @Return 1 if is a valid zone, 0 otherwise
 */
int8_t strat_is_valid_zone(uint8_t robot, int8_t zone_num)
{
	/* return if zone_num out of range */
	if(zone_num < 0 || zone_num >= ZONES_MAX){
		ERROR (E_USER_STRAT, "ERROR, %s, zone_num out of range");
		return 0;
	}

	/* if the zone has 0 priority then must be avoided */
	if(strat_infos.zones[zone_num].prio == 0)
	{
		//DEBUG(E_USER_STRAT,"zone num: %d. avoid.");
		return 0;
	}

	/* discard current zone */
	if(strat_smart[robot].current_zone == zone_num)
	{
		//DEBUG(E_USER_STRAT,"zone num: %d. current_zone.");
		return 0;
	}

	/* discard if opp is in zone */
	if(opponents_are_in_area(COLOR_X(strat_infos.zones[zone_num].x_up), 	strat_infos.zones[zone_num].y_up,
							 COLOR_X(strat_infos.zones[zone_num].x_down),	strat_infos.zones[zone_num].y_down)) {
		return 0;
	}


	return 1;
}


/* return new work zone, -1 if any zone is found */
#define STRAT_NO_MORE_ZONES	-1
#define STRAT_NO_VALID_ZONE	-2
int8_t strat_get_new_zone(uint8_t robot)
{
	uint8_t prio_max = 0;
	int8_t zone_num = STRAT_NO_MORE_ZONES;
	int8_t i=0;

	/* FIXME: never returns NO_MORE_ZONES */

	/* 1. get the robot NO CHECKED zone with the maximun priority  */
	for(i=0; i < ZONES_MAX; i++)
	{
		if(strat_infos.zones[i].prio >= prio_max &&
		  (strat_infos.zones[i].flags != ZONE_CHECKED) &&
		  (strat_infos.zones[i].flags != ZONE_AVOID) &&
		  (strat_infos.zones[i].robot == robot))
		{
			/* check if is a valid zone */
			prio_max = strat_infos.zones[i].prio;
			zone_num = i;
		}
	}

	/* 2. check if the maximum priority zone is valid */
	if(zone_num != STRAT_NO_MORE_ZONES)
	{
		if (!strat_is_valid_zone(robot, zone_num))
			zone_num = STRAT_NO_VALID_ZONE;
	}

	/* XXX: here we have the zone with the maximum priority, and then
			we check if this zone is valid.

			Why we don't discard the no valid zones in the maximun priority
			zone calculation at point 1.
	*/

	return zone_num;
}

#else

/* return 1 if is a valid zone, 0 if not */
uint8_t strat_is_valid_zone(uint8_t robot, int8_t zone_num)
{
	/* return if zone_num out of range */
	if(zone_num < 0 || zone_num >= ZONES_MAX){
		ERROR (E_USER_STRAT, "ERROR, zone_num out of range");
		return 0;
	}

	/* discard priority 0 */
	if(strat_infos.zones[zone_num].prio == 0) {
		return 0;
	}

	/* XXX discard current zone */
	if(strat_smart[robot].current_zone == zone_num &&
	   strat_smart[robot].current_zone != ZONE_MY_STAIRWAY)
	{
		//DEBUG(E_USER_STRAT,"zone num: %d. current_zone.");
		return 0;
	}

	/* discard robot zone */
	if(strat_infos.zones[zone_num].robot != robot) {
		return 0;
	}

	/* discard checked zone */
	if(strat_infos.zones[zone_num].flags & ZONE_CHECKED) {
		return 0;
	}

	/* discard avoid zone */
	if(strat_infos.zones[zone_num].flags & ZONE_AVOID) {
		return 0;
	}

	/* discard one cinema if the other is checked */
	if((zone_num == ZONE_MY_CINEMA_DOWN) && (strat_infos.zones[ZONE_MY_CINEMA_UP].flags & ZONE_CHECKED)) {
		strat_infos.zones[ZONE_MY_CINEMA_DOWN].flags |= ZONE_CHECKED;
		return 0;
	}
	if((zone_num == ZONE_MY_CINEMA_UP) && (strat_infos.zones[ZONE_MY_CINEMA_DOWN].flags & ZONE_CHECKED)) {
		strat_infos.zones[ZONE_MY_CINEMA_UP].flags |= ZONE_CHECKED;
		return 0;
	}
	
	return 1;
}


/* return 1 if opponent is in a zone area, 0 if not */
uint8_t strat_is_opp_in_zone (uint8_t zone_num)
{
	/* check if opponent is in zone area */
	return opponents_are_in_area(COLOR_X(strat_infos.zones[zone_num].x_up), 	strat_infos.zones[zone_num].y_up,
							 	 COLOR_X(strat_infos.zones[zone_num].x_down),	strat_infos.zones[zone_num].y_down);
}

/* return new work zone, STRAT_NO_MORE_ZONES or STRAT_OPP_IS_IN_ZONE */
#define STRAT_NO_MORE_ZONES		-1
#define STRAT_OPP_IS_IN_ZONE	-2
int8_t strat_get_new_zone(uint8_t robot)
{
	uint8_t prio_max = 0;
	int8_t zone_num = STRAT_NO_MORE_ZONES;
	int8_t i=0;

	/* 1. get the valid zone with the maximun priority  */
	for(i=0; i < ZONES_MAX; i++)
	{
		if (strat_is_valid_zone(robot, i) &&
			strat_infos.zones[i].prio >= prio_max)
		{
			prio_max = strat_infos.zones[i].prio;
			zone_num = i;
		}
	}


	/* For secondary robot: check if need to synchronize */
	if(robot==SEC_ROBOT)
	{
		if (strat_smart[SEC_ROBOT].current_zone==ZONE_BLOCK_UPPER_SIDE)
		{
			//Free upper zone if it was still blocking
			zone_num = ZONE_FREE_UPPER_SIDE;
			DEBUG(E_USER_STRAT,"R2, going to ZONE_FREE_UPPER_SIDE.");
		}
		else if(strat_smart_get_msg()==MSG_UPPER_SIDE_FREE)
		{
			DEBUG(E_USER_STRAT,"R2, ZONE_FREE_UPPER_SIDE is FREE.");
			strat_smart_set_msg(MSG_UPPER_SIDE_IS_FREE);
			strat_infos.zones[ZONE_BLOCK_UPPER_SIDE].flags |= ZONE_AVOID;
		}

	}

	/* 2. check if the maximun priority zone is free */
	if(zone_num != STRAT_NO_MORE_ZONES)
	{
		if (strat_is_opp_in_zone(zone_num))
			zone_num = STRAT_OPP_IS_IN_ZONE;
	}

	return zone_num;
}


#endif /* old_version */

/**
 *  main robot: return END_TRAJ if zone is reached or no where to go, err otherwise
 *  secondary robot: return 0 if command SUCESSED, END_TRAJ no where to go, err otherwise
 */
uint8_t strat_goto_zone(uint8_t robot, uint8_t zone_num)
{
	int8_t err=0;

	/* update strat_infos */
	strat_smart[robot].goto_zone = zone_num;

	/* TODO return if -1000  xy */
	if (strat_infos.zones[zone_num].init_x == 0 &&
		strat_infos.zones[zone_num].init_y == 0) {
		WARNING (E_USER_STRAT, "WARNING, No where to GO (xy is NULL)");
		ERROUT(END_TRAJ);
	}

	/* XXX secondary robot: goto and return */
	if(strat_infos.zones[zone_num].robot==SEC_ROBOT) {


		/* specific zones */
		if (zone_num == ZONE_MY_HOME_OUTSIDE) {
			bt_robot_2nd_goto_xy_abs(COLOR_X(strat_infos.zones[zone_num].init_x),
									strat_infos.zones[zone_num].init_y);
		}
		/* normaly we go with avoid */
        else if (zone_num == ZONE_MY_STAIRWAY || zone_num == ZONE_MY_CLAP_3 ||
                 zone_num == ZONE_MY_CINEMA_UP ||  zone_num == ZONE_MY_CINEMA_DOWN)
        {
			DEBUG (E_USER_STRAT, "going backwards");

			/* force go backwards */
			bt_robot_2nd_goto_and_avoid_backward(COLOR_X(strat_infos.zones[zone_num].init_x),
										strat_infos.zones[zone_num].init_y);
        }
		else {
			bt_robot_2nd_goto_and_avoid(COLOR_X(strat_infos.zones[zone_num].init_x),
										strat_infos.zones[zone_num].init_y);
		}

		return 0;
	}

	/* set boundinbox */
	if (zone_num == ZONE_POPCORNCUP_2 || zone_num == ZONE_MY_STAND_GROUP_2 ||
		zone_num == ZONE_MY_CLAP_1 || zone_num == ZONE_MY_CLAP_2 || zone_num == ZONE_MY_CLAP_3)
		strat_set_bounding_box(BOUNDINBOX_WITHOUT_PLATFORM );
	else
		strat_set_bounding_box(BOUNDINBOX_INCLUDES_PLAFORM );


	/* main robot: goto and wait */
	err = goto_and_avoid (COLOR_X(strat_infos.zones[zone_num].init_x),
									strat_infos.zones[zone_num].init_y,
									TRAJ_FLAGS_STD, TRAJ_FLAGS_NO_NEAR);

	/* update strat_infos */
	strat_smart[robot].last_zone = strat_smart[robot].current_zone;
	strat_smart[robot].goto_zone = -1;

	if (!TRAJ_SUCCESS(err))
		strat_smart[robot].current_zone = -1;
	else
		strat_smart[robot].current_zone=zone_num;

end:
	return err;
}


/**
 *  main robot: return END_TRAJ if work is done or no wher to work, err otherwise
 *  secondary robot: return 0 if command SUCESSED, END_TRAJ no where to work, err otherwise
 */
uint8_t strat_work_on_zone(uint8_t robot, uint8_t zone_num)
{
	uint8_t err = END_TRAJ;

	/* TODO: return if -1000 xy */
	if (strat_infos.zones[zone_num].x == 0 &&
		strat_infos.zones[zone_num].y == 0) {
		WARNING (E_USER_STRAT, "%s, WARNING, No where to WORK (xy is NULL)",
				 robot == MAIN_ROBOT? "R1":"R2");
		ERROUT(END_TRAJ);
	}

	/* XXX secondary_robot: send work on zone bt task and return */
	if(strat_infos.zones[zone_num].robot == SEC_ROBOT)
	{
		switch (zone_num)
		{
			case ZONE_POPCORNCUP_1:
				bt_robot_2nd_bt_task_pick_cup (COLOR_X(strat_infos.zones[zone_num].x),
											   strat_infos.zones[zone_num].y);
				break;

			case ZONE_MY_CLAP_3:
				bt_robot_2nd_bt_task_clapperboard(COLOR_X(strat_infos.zones[zone_num].x),
											   strat_infos.zones[zone_num].y);
				break;

			case ZONE_MY_CINEMA_DOWN:
				bt_robot_2nd_bt_task_bring_cup_cinema(COLOR_X(strat_infos.zones[zone_num].x),
											   strat_infos.zones[zone_num].y);
				break;

			case ZONE_MY_CINEMA_UP:
				bt_robot_2nd_bt_task_bring_cup_cinema(COLOR_X(strat_infos.zones[zone_num].x),
											   strat_infos.zones[zone_num].y);
				break;

			case ZONE_MY_STAIRWAY:
				bt_robot_2nd_bt_task_carpet();
				break;

			case ZONE_MY_STAIRS:
				bt_robot_2nd_bt_task_stairs();
				break;

			default:
				ERROR (E_USER_STRAT, "R2, ERROR zone %d not supported", zone_num);
				ERROUT(END_ERROR);

		}

		/* return SUCCESS */
		ERROUT(0);
	}

	/* main robot, dependin on zone */
	switch (zone_num)
	{
		case ZONE_MY_STAND_GROUP_1:

			/* Set start to sec robot */

			strat_smart[MAIN_ROBOT].current_zone = ZONE_MY_STAND_GROUP_1;

			/* TODO: call specific function for stand group 1 */
			err = strat_harvest_orphan_stands (COLOR_X(MY_STAND_4_X),
											   MY_STAND_4_Y,
											   COLOR_INVERT(SIDE_RIGHT),
								 			   COLOR_INVERT(SIDE_RIGHT),
											   0,
											   SPEED_DIST_SLOW, /* harvest speed */
											   0);				/* flags */
		    if (!TRAJ_SUCCESS(err))
			   ERROUT(err);


			/* XXX debug step use only for subtraj command */
			//strat_debug_wait_key_pressed (MAIN_ROBOT);

			err = strat_harvest_orphan_stands (COLOR_X(MY_STAND_5_X),
											   MY_STAND_5_Y,
											   COLOR_INVERT(SIDE_LEFT),
								 			   COLOR_INVERT(SIDE_LEFT),
											   0,
											   SPEED_DIST_SLOW, /* harvest speed */
											   0);				/* flags */
		    if (!TRAJ_SUCCESS(err))
			   ERROUT(err);


			DEBUG(E_USER_STRAT,"R1, sending message START.");
			strat_smart_set_msg(MSG_START);

			/* POPCORNCUP_3 */
			err = strat_harvest_popcorn_cup (COLOR_X(strat_infos.zones[ZONE_POPCORNCUP_3].x),
									   strat_infos.zones[ZONE_POPCORNCUP_3].y,
									   SIDE_FRONT, 0);

			/* XXX debug step use only for subtraj command */
			//strat_debug_wait_key_pressed (MAIN_ROBOT);

			err = strat_harvest_orphan_stands (COLOR_X(MY_STAND_6_X),
											   MY_STAND_6_Y,
											   COLOR_INVERT(SIDE_LEFT),
								 			   COLOR_INVERT(SIDE_LEFT),
											   0,
											   SPEED_DIST_SLOW, /* harvest speed */
											   0);				/* flags */
			break;


		case ZONE_MY_STAND_GROUP_2:
			err = strat_harvest_orphan_stands (COLOR_X(strat_infos.zones[zone_num].x),
											   strat_infos.zones[zone_num].y,
											   COLOR_INVERT(SIDE_LEFT),         /* side target */
								 			   SIDE_ALL,                        /* storing sides */
											   COLOR_A_REL(-10),                /* blade angle */
											   SPEED_DIST_SLOW,                 /* harvest speed */
											   STANDS_HARVEST_BACK_INIT_POS);
			break;

		case ZONE_MY_STAND_GROUP_3:

#if 0
			err = strat_harvest_orphan_stands (COLOR_X(strat_infos.zones[zone_num].x),
											   strat_infos.zones[zone_num].y,
											   COLOR_INVERT(SIDE_LEFT),         /* side target */
								 			   SIDE_ALL,                        /* storing sides */
											   COLOR_A_REL(-20),                /* blade angle */
											   SPEED_DIST_VERY_SLOW,            /* harvest speed */
											   STANDS_HARVEST_BACK_INIT_POS);	/* flags */
#endif
			err = strat_harvest_orphan_stands (COLOR_X(MY_STAND_3_X),
											   MY_STAND_3_Y,
											   COLOR_INVERT(SIDE_LEFT),         /* side target */
								 			   COLOR_INVERT(SIDE_LEFT),        /* storing sides */
											   COLOR_A_REL(0),                /* blade angle */
											   SPEED_DIST_SLOW,            /* harvest speed */
											   STANDS_HARVEST_BACK_INIT_POS);	/* flags */
			break;

		case ZONE_MY_STAND_GROUP_4:

			err = strat_harvest_orphan_stands (COLOR_X(strat_infos.zones[zone_num].x),
											   strat_infos.zones[zone_num].y,
											   COLOR_INVERT(SIDE_RIGHT),        /* side target */
								 			   COLOR_INVERT(SIDE_RIGHT),        /* storing sides */
											   0,                               /* blade angle */
											   SPEED_DIST_SLOW,                 /* harvest speed */
											   STANDS_HARVEST_BACK_INIT_POS |
                                               STANDS_HARVEST_CALIB_X);	        /* flags */

			break;

		case ZONE_MY_HOME_POPCORNS:

			err = strat_release_popcorns_in_home (COLOR_X(strat_infos.zones[zone_num].x),
													strat_infos.zones[zone_num].y, 0);
			break;

		case ZONE_MY_HOME_SPOTLIGHT:

			err = strat_buit_and_release_spotlight (COLOR_X(strat_infos.zones[zone_num].x),
													strat_infos.zones[zone_num].y,
													COLOR_INVERT(SIDE_LEFT));
			break;

		case ZONE_POPCORNCUP_1:
			err = strat_harvest_popcorn_cup (COLOR_X(strat_infos.zones[zone_num].x),
									   strat_infos.zones[zone_num].y,
									   SIDE_REAR, 0);
			break;


		case ZONE_POPCORNCUP_2:
			err = strat_harvest_popcorn_cup (COLOR_X(strat_infos.zones[zone_num].x),
									   strat_infos.zones[zone_num].y,
									   SIDE_REAR, 0);
			break;

		case ZONE_POPCORNCUP_3:
			err = strat_harvest_popcorn_cup (COLOR_X(strat_infos.zones[zone_num].x),
									   strat_infos.zones[zone_num].y,
									   SIDE_FRONT, 0);
			break;


		case ZONE_MY_CLAP_1:
			err = strat_close_clapperboards (COLOR_X(strat_infos.zones[zone_num].x),
									   strat_infos.zones[zone_num].y,
									   COLOR_INVERT(SIDE_RIGHT), 0);
			break;

		case ZONE_MY_CLAP_2:
			err = strat_close_clapperboards (COLOR_X(strat_infos.zones[zone_num].x),
									   strat_infos.zones[zone_num].y,
									   COLOR_INVERT(SIDE_RIGHT), 0);
			break;

		case ZONE_MY_CLAP_3:
			err = strat_close_clapperboards (COLOR_X(strat_infos.zones[zone_num].x),
									   strat_infos.zones[zone_num].y,
									   COLOR_INVERT(SIDE_LEFT), 0);
			break;


		case ZONE_MY_POPCORNMAC:

			err = strat_harvest_popcorns_machine (COLOR_X(strat_infos.zones[zone_num].x),
									   strat_infos.zones[zone_num].y);
			break;

		case ZONE_OPP_POPCORNMAC:
			err = strat_harvest_popcorns_machine (COLOR_X(strat_infos.zones[zone_num].x),
									   strat_infos.zones[zone_num].y);
			break;

		case ZONE_MY_CINEMA_UP:
			err = strat_release_popcorns_in_home (COLOR_X(strat_infos.zones[zone_num].x),
													strat_infos.zones[zone_num].y, POPCORN_ONLY_CUP);
			break;

		case ZONE_MY_CINEMA_DOWN:
			err = strat_release_popcorns_in_home (COLOR_X(strat_infos.zones[zone_num].x),
													strat_infos.zones[zone_num].y, POPCORN_ONLY_CUP);
			break;

		/* not yet or don't know how to work in the zones */
		case ZONE_MY_STAIRS:
		case ZONE_MY_STAIRWAY:

			DEBUG(E_USER_STRAT, "R1, Working on zone ... ");
			trajectory_turnto_xy (&mainboard.traj,
								  COLOR_X(strat_infos.zones[zone_num].x),
								  strat_infos.zones[zone_num].y);
			err = wait_traj_end(TRAJ_FLAGS_NO_NEAR);

			//time_wait_ms(2000);
			DEBUG(E_USER_STRAT, "R1, ... fishish!! ");
			//ERROUT(END_TRAJ);
			break;

		default:
			ERROR (E_USER_STRAT, "R1, ERROR zone %d not supported", zone_num);
			//ERROUT(END_TRAJ);
			break;
	}

end:
	return err;
}





/* return 1 if need to wait SYNCHRONIZATION */
uint8_t strat_wait_sync_main_robot(void)
{
    /* XXX HACK */
    //return 0;

	/* manual syncro */
	if (strat_infos.debug_step)
	{
        /* key trigger */
        if (strat_smart[MAIN_ROBOT].key_trigger) {
            strat_smart[MAIN_ROBOT].key_trigger = 0;
            return 0;
		}
        else
            return 1;
	}

	/* strat syncro */
	/* Wait until "is free" from sec robot */
	//if(strat_smart_get_msg() != MSG_UPPER_SIDE_IS_FREE)
	//	return 1;

    return 0;
}


/* implements the strategy motor of the main robot,
   XXX: needs to be called periodically, BLOCKING implementation */
uint8_t strat_smart_main_robot(void)
{
	int8_t zone_num;
	uint8_t err;
	static uint8_t no_more_zones;
     static microseconds us;
	strat_strategy_time();

	/* get new zone */
	zone_num = strat_get_new_zone(MAIN_ROBOT);

	/* if zone is on upper side, send sec robot to free the way */
	if(zone_num == ZONE_MY_STAND_GROUP_3 || zone_num == ZONE_MY_STAND_GROUP_4 || zone_num == ZONE_MY_POPCORNMAC)
	{
		if(strat_smart_get_msg() != MSG_UPPER_SIDE_IS_FREE)
		{
			//DEBUG(E_USER_STRAT,"R1, sending message MSG_UPPER_SIDE_FREE.");
			strat_smart_set_msg(MSG_UPPER_SIDE_FREE);

			/* SYNCHRONIZATION mechanism */
			if(strat_wait_sync_main_robot())
			{
			#if 0
				if (time_get_us2()-us > 10000000) {
					DEBUG(E_USER_STRAT,"R1, WAITING sync");
					if (strat_infos.debug_step)
						DEBUG(E_USER_STRAT,"R1, press key 'p' for continue");
					us = time_get_us2();
				}
			#endif
				return END_TRAJ;
			}
			else
				DEBUG(E_USER_STRAT,"R1, going to ZONE_UPPER_SIDE seems to be free. Going.");
		}
	}

	/* if no more zones return */
	if (zone_num == STRAT_NO_MORE_ZONES) {

		if (!no_more_zones) {
			no_more_zones = 1;
			DEBUG(E_USER_STRAT,"R1, strat #%d, NO MORE ZONES", strat_smart[MAIN_ROBOT].current_strategy);
		}
		return END_TRAJ;
	}

	/* if no valid zone, change strategy and return */
	if (zone_num == STRAT_OPP_IS_IN_ZONE) {
		DEBUG(E_USER_STRAT,"R1, strat #%d, OPPONENT IN ZONE", strat_smart[MAIN_ROBOT].current_strategy);
		strat_set_next_main_strategy();
		return END_TRAJ;
	}

	DEBUG(E_USER_STRAT,"R1, strat #%d: get zone %s (%d, %d)",
						strat_smart[MAIN_ROBOT].current_strategy,
						get_zone_name(zone_num), zone_num, strat_infos.zones[zone_num].prio);

	/* XXX debug step use only for subtraj command */
	//strat_debug_wait_key_pressed (MAIN_ROBOT);

	/* goto zone */
	DEBUG(E_USER_STRAT,"R1, strat #%d: goto zone %s (%d, %d)",
						strat_smart[MAIN_ROBOT].current_strategy,
						get_zone_name(zone_num), zone_num, strat_infos.zones[zone_num].prio);


	/* goto, if can't reach the zone change the strategy and return */
	err = strat_goto_zone(MAIN_ROBOT, zone_num);
	if (!TRAJ_SUCCESS(err)) {
		DEBUG(E_USER_STRAT,"R1, ERROR, goto returned %s", get_err(err));
		strat_set_next_main_strategy();
		return err;
	}

	/* XXX debug step use only for subtraj command */
	//strat_debug_wait_key_pressed (MAIN_ROBOT);

	/* work on zone */
	DEBUG(E_USER_STRAT,"R1, strat #%d: work on zone %s (%d, %d)",
						strat_smart[MAIN_ROBOT].current_strategy,
						get_zone_name(zone_num), zone_num, strat_infos.zones[zone_num].prio);

	/* work */
	err = strat_work_on_zone(MAIN_ROBOT, zone_num);
	if (!TRAJ_SUCCESS(err)) {
		DEBUG(E_USER_STRAT,"R1, ERROR, work returned %s", get_err(err));
		/* XXX should not happen, return END_TRAJ */
		err = END_TRAJ;

		/* special case */
		if (zone_num == ZONE_MY_STAND_GROUP_1)
			return err;

	}

	/* check the zone as DONE */
	strat_infos.zones[zone_num].flags |= ZONE_CHECKED;
	return err;
}


void strat_smart_set_msg (uint8_t msg)
{
	uint8_t flags;

	IRQ_LOCK(flags);
	strat_infos.msg = msg;
	IRQ_UNLOCK(flags);
}


uint8_t strat_smart_get_msg (void)
{
	uint8_t flags;
	uint8_t ret;

	IRQ_LOCK(flags);
	ret=strat_infos.msg;
	IRQ_UNLOCK(flags);

	return ret;
}



/* return 1 if need to wait SYNCHRONIZATION */
uint8_t strat_wait_sync_secondary_robot(void)
{
	/* manual syncro */
	if (strat_infos.debug_step)
	{
        /* key capture */
    	int16_t c;
        c = cmdline_getchar();
        if ((char)c == 'p')
            strat_smart[MAIN_ROBOT].key_trigger = 1;
        else if ((char)c == 't')
            strat_smart[SEC_ROBOT].key_trigger = 1;


        if (strat_smart[SEC_ROBOT].key_trigger) {
            strat_smart[SEC_ROBOT].key_trigger = 0;
            return 0;
		}
        else
            return 1;
	}

	/* strat syncro */
	/* Block until main robot sets start */
	if (strat_smart_get_msg()==MSG_WAIT_START)
		return 1;

	/* Block upper side until "free" message (or timeout) */
#define ZONE_UPPER_SIDE_BLOCKING_TIMEOUT 30

	if ((strat_smart_get_msg() == MSG_UPPER_SIDE_IS_BLOCKED) &&
		(strat_smart[SEC_ROBOT].current_zone == ZONE_BLOCK_UPPER_SIDE) &&
		(time_get_s() < ZONE_UPPER_SIDE_BLOCKING_TIMEOUT))
	{
		return 1;
	}

    return 0;
}


/* implements the strategy motor of the secondary robot,
   XXX: needs to be called periodically, NON-BLOCKING implementation */
uint8_t strat_smart_secondary_robot(void)
{
    	/* static states */
	#define SYNCHRONIZATION 	0
	#define GET_NEW_ZONE	1
	#define GOTO			2
	#define GOTO_WAIT_ACK	3
	#define GOTO_WAIT_END	4
	#define WORK			5
	#define WORK_WAIT_ACK	6
	#define WORK_WAIT_END	7

	static microseconds us = 0;
	uint8_t received_ack;
	uint8_t err=0;

	static int8_t zone_num=STRAT_NO_MORE_ZONES;
	static uint8_t no_more_zones = 0;
	static uint8_t state = SYNCHRONIZATION;
#ifdef DEBUG_STRAT_SECONDARY
	static uint8_t state_saved = 0xFF;

	/* transitions debug */
	if (state != state_saved) {
		state_saved = state;
		DEBUG(E_USER_STRAT,"R2, new state is %d", state);
	}
#endif

	/* strat smart state machine implementation */
	switch (state)
	{
		case SYNCHRONIZATION:
            /* SYNCHRONIZATION mechanism */
            if(strat_wait_sync_secondary_robot())
            {
            #if 0
                if (time_get_us2()-us > 10000000) {
                    DEBUG(E_USER_STRAT,"R2, WAITING syncro");
					if (strat_infos.debug_step)
            			DEBUG(E_USER_STRAT,"R2, press key 't' for continue");
                    us = time_get_us2();
                }
            #endif
				break;
            }

            /* next state */
            state = GET_NEW_ZONE;

#ifdef DEBUG_STRAT_SECONDARY
			state_saved = state;
			DEBUG(E_USER_STRAT,"R2, new state is %d", state);
#endif
			/* XXX: continue without break */
			//break;

		case GET_NEW_ZONE:
			zone_num = strat_get_new_zone(SEC_ROBOT);

			/* if no more zones, goto SYNCHRONIZATION state XXX???*/
			if(zone_num == STRAT_NO_MORE_ZONES ) {
				if (!no_more_zones) {
					no_more_zones = 1;
					DEBUG(E_USER_STRAT,"R2, strat #%d, NO MORE ZONES", strat_smart[SEC_ROBOT].current_strategy);
				}
				state = SYNCHRONIZATION;
				break;
			}

			/* if no valid zone, change strategy */
 			if(zone_num == STRAT_OPP_IS_IN_ZONE) {
				DEBUG(E_USER_STRAT,"R2, strat #%d, NO VALID ZONE", strat_smart[SEC_ROBOT].current_strategy);
				//DEBUG(E_USER_STRAT,"R2, strat #%d, OPPONENT IN ZONE", strat_smart[SEC_ROBOT].current_strategy);
				strat_set_next_sec_strategy();
				break;
			}


			DEBUG(E_USER_STRAT,"R2, strat #%d: get zone %s (%d, %d)",
						strat_smart[SEC_ROBOT].current_strategy,
						get_zone_name(zone_num), zone_num, strat_infos.zones[zone_num].prio);


			/* update statistics */
			strat_smart[SEC_ROBOT].goto_zone = zone_num;

			/* next state */
			state = GOTO;

#ifdef DEBUG_STRAT_SECONDARY
			state_saved = state;
			DEBUG(E_USER_STRAT,"R2, new state is %d", state);
#endif
			/* XXX: continue without break */
			//break;

		case GOTO:
			/* goto zone */
			DEBUG(E_USER_STRAT,"R2, strat #%d: goto zone %s (%d, %d)",
						strat_smart[SEC_ROBOT].current_strategy,
						get_zone_name(zone_num), zone_num, strat_infos.zones[zone_num].prio);

			err = strat_goto_zone(SEC_ROBOT, zone_num);

            /* END_TRAJ means "no where to go", directly work */
            if (TRAJ_SUCCESS(err)) {
                state = WORK;
                break;
            }else if (err) {
				/* XXX never shoud be reached, infinite loop */
				DEBUG(E_USER_STRAT,"R2, ERROR, goto returned %s at line %d", get_err(err), __LINE__);
				//set new strategy
				strat_set_next_sec_strategy();
				state = GET_NEW_ZONE;
				break;
			}

			/* next state */
			state = GOTO_WAIT_ACK;
			us = time_get_us2();
			break;

		case GOTO_WAIT_ACK:
		    /* return if no minimum time */
			if (time_get_us2() - us < 200000L)
				break;

			/* wait ACK value until ACK, NACK or timeout */
			received_ack = bt_robot_2nd_is_ack_received ();

			if(received_ack == 1)
			{
				/* ACK, wait end trajectory */
				us = time_get_us2();
				state = GOTO_WAIT_END;
			}
			else if (received_ack !=1 && received_ack != 0) {
				/* NACK, retry */
				state = GET_NEW_ZONE;
			}
			else if (time_get_us2() - us > 1000000L) {
				/* timeout, retry */
				state = GET_NEW_ZONE;
			}
			break;

		case GOTO_WAIT_END:
		    /* return if no minimum time */
			if (time_get_us2() - us < 200000L)
				break;

			/* goto zone, if can't reach the zone change the strategy and get new one */
			if(!bt_robot_2nd_is_ret_received())
				break;

			err = bt_robot_2nd_test_end();
			
			if (!TRAJ_SUCCESS(err)) {
				DEBUG(E_USER_STRAT,"R2, ERROR, goto returned %s", get_err(err));
				strat_smart[SEC_ROBOT].current_zone = -1; /* TODO: why? */
				strat_set_next_sec_strategy();
				state = GET_NEW_ZONE;
				break;
			}

			/* update statistics */
			strat_smart[SEC_ROBOT].last_zone = strat_smart[SEC_ROBOT].current_zone;
			strat_smart[SEC_ROBOT].current_zone = strat_smart[SEC_ROBOT].goto_zone;

			/* send message after done synchronization */
			if(strat_smart[SEC_ROBOT].current_zone == ZONE_BLOCK_UPPER_SIDE)
			{
				DEBUG(E_USER_STRAT,"R2, in ZONE_BLOCK_UPPER_SIDE.");
				strat_smart_set_msg(MSG_UPPER_SIDE_IS_BLOCKED);

			}
			else if(strat_smart[SEC_ROBOT].current_zone == ZONE_FREE_UPPER_SIDE)
			{
				DEBUG(E_USER_STRAT,"R2, in ZONE_FREE_UPPER_SIDE.");
				strat_smart_set_msg(MSG_UPPER_SIDE_IS_FREE);
			}

			/* next state */
			if((strat_smart[SEC_ROBOT].current_zone == ZONE_BLOCK_UPPER_SIDE) ||
				(strat_smart[SEC_ROBOT].current_zone == ZONE_FREE_UPPER_SIDE))
			{
				/* update statistics */
				strat_infos.zones[zone_num].flags |= ZONE_CHECKED;
				state = SYNCHRONIZATION;
			}

			else
			{
				state = WORK;
			}

#ifdef DEBUG_STRAT_SECONDARY
			state_old = state;
			DEBUG(E_USER_STRAT,"R2, new state is %d", state);
#endif
			// Must do break
			break;

		case WORK:

			/* FIXME: strat debug on an event */
			//strat_debug_wait_key_pressed (SEC_ROBOT);

			/* work */
			DEBUG(E_USER_STRAT,"R2, strat #%d: work on zone %s (%d, %d)",
						strat_smart[SEC_ROBOT].current_strategy,
						get_zone_name(zone_num), zone_num, strat_infos.zones[zone_num].prio);

			err = strat_work_on_zone(SEC_ROBOT, zone_num);

            /* END_TRAJ means "no where to work", check zone and go directly to synchronize*/
            if (TRAJ_SUCCESS(err)) {
		        /* update statistics */
		        strat_infos.zones[zone_num].flags |= ZONE_CHECKED;

                /* next state */
                state = SYNCHRONIZATION;
			    break;
            }
			else if (err) {
				/* XXX never shoud be reached, infinite loop */
				DEBUG(E_USER_STRAT,"R2, ERROR, (case work) work returned %s at line %d", get_err(err), __LINE__);
				state = GET_NEW_ZONE;
				break;
			}

			/* next state */
			state = WORK_WAIT_ACK;
			us = time_get_us2();
			break;

		case WORK_WAIT_ACK:
		    /* return if no minimum time */
			if (time_get_us2() - us < 200000L)
				break;

			/* wait ACK value until ACK, NACK or timeout */
			received_ack = bt_robot_2nd_is_ack_received ();

			if(received_ack == 1)
			{
				/* ACK, wait end work */
				us = time_get_us2();
				state = WORK_WAIT_END;
			}
			else if (received_ack !=1 && received_ack != 0) {
				/* NACK, retry */
				state = GET_NEW_ZONE;
			}
			else if (time_get_us2() - us > 1000000L) {
				/* timeout, retry */
				state = GET_NEW_ZONE;
			}

			break;

		case WORK_WAIT_END:
		    /* return if no minimum time */
			if (time_get_us2() - us < 200000L)
				break;

			if(!bt_robot_2nd_is_ret_received())
				break;

			err = bt_robot_2nd_test_end();

			if (!TRAJ_SUCCESS(err)) {
				DEBUG(E_USER_STRAT,"R2, ERROR, work returned %s.", get_err(err));
				
				//If there is an error working in the cinemas don't try it again
				if(strat_smart[SEC_ROBOT].current_zone == ZONE_MY_CINEMA_DOWN
				|| strat_smart[SEC_ROBOT].current_zone == ZONE_MY_CINEMA_UP){
					strat_infos.zones[zone_num].flags |= ZONE_CHECKED;
				}

				/* timeout. After timeout, change strategy */
				if (time_get_us2() - us > 5000000L) {
					DEBUG(E_USER_STRAT,"R2, changing strategy.");
					strat_set_next_sec_strategy();
				}
				state = GET_NEW_ZONE;
				break;
			}

			/* update statistics */
			strat_infos.zones[zone_num].flags |= ZONE_CHECKED;

            /* next state */
            state = SYNCHRONIZATION;
			break;


		default:
            state = SYNCHRONIZATION;
			break;
	}

	return err;
}

void strat_opp_tracking (void)
{
#if 0
#define MAX_TIME_BETWEEN_VISITS_MS	4000
#define TIME_MS_TREE				1500
#define TIME_MS_HEART				1500
#define TIME_MS_BASKET				1000
#define UPDATE_ZONES_PERIOD_MS		25

	uint8_t flags;
	uint8_t zone_opp;

    /* check if there are opponents in every zone */
    for(zone_opp = 0; zone_opp <  ZONES_MAX-1; zone_opp++)
    {

	if(opponents_are_in_area(COLOR_X(strat_infos.zones[zone_opp].x_up), strat_infos.zones[zone_opp].y_up,
                                     COLOR_X(strat_infos.zones[zone_opp].x_down), strat_infos.zones[zone_opp].y_down)){

			if(!(strat_infos.zones[zone_opp].flags & (ZONE_CHECKED_OPP)))
			{
				IRQ_LOCK(flags);
				strat_infos.zones[zone_opp].last_time_opp_here=time_get_us2();
				IRQ_UNLOCK(flags);
				if((time_get_us2() - strat_infos.zones[zone_opp].last_time_opp_here) < MAX_TIME_BETWEEN_VISITS_MS*1000L)
				{
					/* Opponent continues in the same zone: */
					/* update zone time */
					IRQ_LOCK(flags);
					strat_infos.zones[zone_opp].opp_time_zone_us += UPDATE_ZONES_PERIOD_MS*1000L;
					IRQ_UNLOCK(flags);

					/* Mark zone as checked and sum points */
					switch(strat_infos.zones[zone_opp].type)
					{
						case ZONE_TYPE_TREE:
							if(strat_infos.zones[zone_opp].opp_time_zone_us>=TIME_MS_TREE*1000L)
							{
								strat_infos.zones[zone_opp].flags |= ZONE_CHECKED_OPP;
								strat_infos.opp_harvested_trees++;
								DEBUG(E_USER_STRAT,"opp_harvested_trees=%d",strat_infos.opp_harvested_trees);
								DEBUG(E_USER_STRAT,"OPP approximated score: %d", strat_infos.opp_score);
							}
							break;
						case ZONE_TYPE_BASKET:
							if(((mainboard.our_color==I2C_COLOR_YELLOW) && (zone_opp==ZONE_BASKET_1)) ||
							((mainboard.our_color==I2C_COLOR_GREEN) && (zone_opp==ZONE_BASKET_2)))
							{
								if(strat_infos.zones[zone_opp].opp_time_zone_us>=TIME_MS_BASKET*1000L)
								{
									if(strat_infos.opp_harvested_trees!=0)
									{
										strat_infos.opp_score += strat_infos.opp_harvested_trees * 3;
										strat_infos.opp_harvested_trees=0;
										DEBUG(E_USER_STRAT,"opp_harvested_trees=%d",strat_infos.opp_harvested_trees);
										DEBUG(E_USER_STRAT,"OPP approximated score: %d", strat_infos.opp_score);
									}
								}
							}
							break;
						case ZONE_TYPE_HEART:
							if(strat_infos.zones[zone_opp].opp_time_zone_us>= TIME_MS_HEART*1000L)
							{
								strat_infos.zones[zone_opp].flags |= ZONE_CHECKED_OPP;
								strat_infos.opp_score += 4;
								DEBUG(E_USER_STRAT,"OPP approximated score: %d", strat_infos.opp_score);
							}
							break;
						default:
							break;
					}
				}

				/* Zone has changed */
				else
				{
					/* reset zone time */
					IRQ_LOCK(flags);
					strat_infos.zones[zone_opp].opp_time_zone_us = 0;
					IRQ_UNLOCK(flags);
				}
			}
		}
	}
#endif
}
