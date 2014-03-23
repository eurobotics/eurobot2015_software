/*
 *  Copyright Droids Corporation (2007)
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
 *  Revision : $Id: i2c_commands.h,v 1.9 2009/05/27 20:04:06 zer0 Exp $
 *
 */

/*  
 *  Copyright Robotics Association of Coslada, Eurobotics Engineering (2011)
 *  Javier Bali�as Santos <javier@arc-robots.org>
 *
 *  Code ported to family of microcontrollers dsPIC from
 *  i2c_commands.h,v 1.9 2009/05/27 20:04:06 zer0 Exp.
 */

#ifndef _I2C_COMMANDS_H_
#define _I2C_COMMANDS_H_


#define I2C_SLAVEDSPIC_ADDR 	0x10
#define I2C_GPIOS_01_ADDR 	  0x20
#define I2C_GPIOS_23_ADDR 	  0x21

#define I2C_COLOR_RED 	  0
#define I2C_COLOR_YELLOW	1
#define I2C_COLOR_MAX		  2

#define I2C_OPPONENT_NOT_THERE (-1000)

#define I2C_SIDE_LEFT   1
#define I2C_SIDE_RIGHT  2
#define I2C_SIDE_ALL    3

#define I2C_LEVEL_GROUND_FIRE_0     0
#define I2C_LEVEL_GROUND_FIRE_1     0
#define I2C_LEVEL_GROUND_FIRE_2     0
#define I2C_LEVEL_UP_FIRE_TOP       1
#define I2C_LEVEL_UP_FIRE_CENTER    2
#define I2C_LEVEL_TORCH_FIRE_0
#define I2C_LEVEL_TORCH_FIRE_1
#define I2C_LEVEL_TORCH_FIRE_2
#define I2C_LEVEL_TORCH_FIRE_3
#define I2C_LEVEL_HEART_FIRE_0
#define I2C_LEVEL_HEART_FIRE_1
#define I2C_LEVEL_HEART_FIRE_2
#define I2C_LEVEL_FRUIT_BASKET_0
#define I2C_LEVEL_FRUIT_BASKET_1
#define I2C_LEVEL_FRUIT_BASKET_2
#define I2C_LEVEL_FRUIT_BASKET_3
#define I2C_LEVEL_FRUIT_BASKET_4


struct i2c_cmd_hdr {
	uint8_t cmd;
};


/* shared between robots */
#define WAIT_FOR_MAIN_ROBOT_TIMEOUT 60

/****/
/* request status of boards and gpios (read data) */
/****/

#define I2C_REQ_GPIOS_STATUS 			0x00
#define I2C_ANS_GPIOS_STATUS 			0x00
struct i2c_gpios_status {
	int8_t gpio0;	
	int8_t gpio1;	
};

#define I2C_REQ_SLAVEDSPIC_STATUS 0x01
#define I2C_ANS_SLAVEDSPIC_STATUS 0x01
struct i2c_slavedspic_status{
	struct i2c_cmd_hdr hdr;


  /* TODO */
	
  /* actuators */

	/* rd sensors */

  /* wr sensors */

	/* infos */
	uint8_t status;
#define I2C_SLAVEDSPIC_STATUS_BUSY		1
#define I2C_SLAVEDSPIC_STATUS_READY		0

	uint8_t fire_mode;
	uint8_t fruit_mode;
	uint8_t stick_mode;

  /* statistics */
	int8_t nb_fires_l;
	int8_t nb_fires_r;
};


/****/
/* commands to boards (write data) */
/****/

#define I2C_CMD_GENERIC				0x00

#define I2C_CMD_LED_CONTROL		0x01
struct i2c_cmd_led_control{
	struct i2c_cmd_hdr hdr;
	uint8_t led_num:7;
	uint8_t state:1;	
};

#define I2C_CMD_SLAVEDSPIC_SET_MODE 0x02
struct i2c_cmd_slavedspic_set_mode {
	struct i2c_cmd_hdr hdr;
	
#define I2C_SLAVEDSPIC_MODE_INIT		            0x01
#define I2C_SLAVEDSPIC_MODE_POWER_OFF		      0x02


#define I2C_SLAVEDSPIC_MODE_BOOT_TRAY				0x03
#define I2C_SLAVEDSPIC_MODE_BOOT_DOOR				0x04
#define I2C_SLAVEDSPIC_MODE_COMBS					0x05
#define I2C_SLAVEDSPIC_MODE_TREE_TRAY				0x06
#define I2C_SLAVEDSPIC_MODE_STICK					0x07


#if 0
#define I2C_SLAVEDSPIC_MODE_PREP_PICKUP_FIRE	  0x03
#define I2C_SLAVEDSPIC_MODE_PICKUP_FIRE         0x04
#define I2C_SLAVEDSPIC_MODE_STORE_FIRE          0x04
#define I2C_SLAVEDSPIC_MODE_LOAD_FIRE           0x04
#define I2C_SLAVEDSPIC_MODE_PREP_DRAG_FIRE      0x04
#define I2C_SLAVEDSPIC_MODE_DRAG_FIRE           0x04
#define I2C_SLAVEDSPIC_MODE_PREP_TURN_FIRE      0x04
#define I2C_SLAVEDSPIC_MODE_TURN_FIRE           0x04
#define I2C_SLAVEDSPIC_MODE_PREP_TURN_FIRE      0x04

#define I2C_SLAVEDSPIC_MODE_HIDE_ARMS           0x04

#define I2C_SLAVEDSPIC_MODE_PREP_HARVEST_TREE   0x04
#define I2C_SLAVEDSPIC_MODE_HARVEST_TREE        0x04


#define I2C_SLAVEDSPIC_MODE_PREP_TOXIC_FRUIT    0x04
#define I2C_SLAVEDSPIC_MODE_PICKUP_TOXIC_FRUIT  0x04
#define I2C_SLAVEDSPIC_MODE_DRAG_TOXIC_FRUIT    0x04
#endif

	uint8_t mode;
	union{

		struct {
			uint8_t mode;
#define I2C_BOOT_DOOR_MODE_OPEN		0
#define I2C_BOOT_DOOR_MODE_CLOSE		1
		} boot_door;

		struct {
			uint8_t mode;
#define I2C_BOOT_TRAY_MODE_DOWN		0
#define I2C_BOOT_TRAY_MODE_VIBRATE	1
		} boot_tray;

		struct {
			uint8_t mode;
#define I2C_COMBS_MODE_HIDE				0
#define I2C_COMBS_MODE_OPEN				1
#define I2C_COMBS_MODE_HARVEST_CLOSE	2
#define I2C_COMBS_MODE_HARVEST_OPEN		3

			int8_t offset;
		} combs;

		struct {
			uint8_t mode;
#define I2C_TREE_TRAY_MODE_OPEN 			0	
#define I2C_TREE_TRAY_MODE_CLOSE 		1		
#define I2C_TREE_TRAY_MODE_HARVEST		2	

			int8_t offset;
		} tree_tray;

		struct {
			uint8_t type;
#define I2C_STICK_TYPE_RIGHT	0
#define I2C_STICK_TYPE_LEFT	1	

			uint8_t mode;
#define I2C_STICK_MODE_HIDE					0
#define I2C_STICK_MODE_PUSH_FIRE				1
#define I2C_STICK_MODE_PUSH_TORCH_FIRE		2
#define I2C_STICK_MODE_CLEAN_FLOOR			3
#define I2C_STICK_MODE_CLEAN_HEART			4

			int8_t offset;
		} stick;

		/* add more here */
	};
};

#endif
