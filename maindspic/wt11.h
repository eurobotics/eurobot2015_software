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

#ifndef __WT11_H__
#define __WT11_H__

#if 0

/* UART used for send and receive data */
#define BT_UART   MUX_UART

/* number of bt devices, maximun 4 */
#define BT_PROTO_NUM_DEVICES 2

/* send and received modes */
#define WT11_MODE_NORMAL  0
#define WT11_MODE_MUX     1

/* maximun size of message */
#define WT11_MUX_LENGTH_MAX 128

/* send data in norma mode, any protocol is used */
void wt11_send (uint8_t *data, uint16_t length);

/* send data using multiplexing mode protocol */
void wt11_send_mux (uint8_t link_id, uint8_t *data, uint16_t length);

/* receive data using multiplexing mode protocol, 
   returns data length, -1 if no data received */
int16_t wt11_recv_mux (uint8_t *link_id, uint8_t *data);

/* receive data in data mode, 
   returns data length, -1 if no data received */
int16_t wt11_rdline (uint8_t *buff, uint16_t buff_size);

/* open serial link in multiplexin mode */
void __wt11_open_link(uint8_t mode, uint8_t *addr, uint8_t *link_id);

inline void wt11_open_link(uint8_t *addr, uint8_t *link_id) {
  __wt11_open_link(WT11_MODE_NORMAL, addr, link_id);
}

inline void wt11_open_link_mux(uint8_t *addr, uint8_t *link_id) {
  __wt11_open_link(WT11_MODE_MUX, addr, link_id);
}

/* close serial link */
void __wt11_close_link(uint8_t mode, uint8_t link_id);

inline void wt11_close_link(uint8_t link_id) {
  __wt11_close_link(WT11_MODE_NORMAL, link_id);
}

inline void wt11_close_link_mux(uint8_t link_id) {
  __wt11_close_link(WT11_MODE_MUX, link_id);
}

/* enable multiplexing mode */
void wt11_enable_mux_mode (void);

/* disable multiplexing mode */
void wt11_disable_mux_mode (void);

/* reset wt11 */
void __wt11_reset (uint8_t mode);

inline void wt11_reset (void) {
  __wt11_reset ((uint8_t)WT11_MODE_NORMAL);
}

inline void wt11_reset_mux (void) {
  __wt11_reset ((uint8_t)WT11_MODE_MUX);
}
#endif

#endif /* __WT11_H__ */
