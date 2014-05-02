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

#ifndef __BT_PROTOCOL_H__
#define __BT_PROTOCOL_H__

/* number of bt devices, maximun 4 */
#define BT_PROTO_NUM_DEVICES 2

/* send and receive commands to/from bt devices, periodic dev status pulling */
void bt_protocol (void);

/************************************************************
 * BEACON BINARY COMMANDS 
 ***********************************************************/


/************************************************************
 * BEACON ASCII COMMANDS 
 ***********************************************************/

/* set color */
void bt_beacon_set_color (void);

/* beacon on */
void bt_beacon_set_on (void);

/* beacon on with watchdog */
void bt_beacon_set_on_watchdog (void);

/* beacon off*/
void bt_beacon_set_off (void);

/* request opponent position */
void bt_beacon_req_status(void);

#endif
