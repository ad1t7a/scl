/* This file is part of scl, a control and simulation library
for robots and biomechanical models.

scl is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

Alternatively, you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License, or (at your option) any later version.

scl is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License and a copy of the GNU General Public License along with
scl. If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * \file CSensoray3DofIO.hpp
 *
 *  Created on: Oct 21, 2012
 *
 *  Author: Samir Menon <smenon@stanford.edu>
 */

#ifndef CSENSORAY3DOFIO_HPP_
#define CSENSORAY3DOFIO_HPP_

//For compiling the demo in cpp instead of c.
#ifdef __cplusplus
extern "C" {
#endif

#include "app2600.h"    // Linux api to 2600 middleware

#ifdef __cplusplus
}
#endif

#include <termios.h>
#include <string>

namespace sensoray
{
  /** A bunch of constants used by the Sensoray driver
   * to establish 3dof I/O */
  class SSensoray3DofIO
  {
  public:
    /** Default constructor : Sets stuff to defaults */
    SSensoray3DofIO() :
      mm_ip_addr_("10.10.10.1"),
      mm_handle_(0),
      timeout_gateway_ms_(100),
      timeout_comport_ms_(100),
      retries_com_(50),
      retries_gateway_(50),
      s2620_channel_width_(0),
      s2620_channel_freq_(1),
      s2620_channel_pwm_(2),
      s2620_channel_encoder_(3),
      com_src_a_(LOGDEV_COM2),
      com_dest_a_(LOGDEV_COM1),
      com_baud_a_(SIO_BR_9600),
      com_src_b_(LOGDEV_COM4),
      com_dest_b_(LOGDEV_COM3),
      com_baud_b_(SIO_BR_115200),
      com_reject_ignore_(0),
      com_reject_evaluate_ (1),iters_ctrl_loop_(0),
      num_iom_boards_(0),
      iom_link_flags_(0),
      interlock_flags_(0),
      num_relay_states_(0)
    {//Somewhat redundant initialization to maintain declaration order
      int i;
      for (i=0; i<16; ++i)
      {
        iom_types_[i] = 0;
        iom_status_[i] = 0;
        s2608_num_aouts_at_iom_[i] = 0;
      }

      for (i=0; i<6; ++i)
      { num_digital_in_states_[i] = 0;  }

      for (i=0; i<16; ++i)
      { analog_in_voltages_[i] = 0;  }

      for (i=0; i<MAX_NUM_AOUTS; ++i)
      { analog_out_voltages_[i] = 0;  }

      for (i=0; i<4; ++i)
      {
        counter_counts_[i] = 0;
        counter_timestamp_[i] = 0;
      }

    }

    /** Default destructor : Does nothing */
    ~SSensoray3DofIO(){}

    // CONSTANTS //////////////////////////////////////////////////////////////////
    /** Set this to the MM's IP address.*/
    const std::string mm_ip_addr_;
    /** This is the first MM in the system, so it is number 0.*/
    const int mm_handle_;

    /** This many milliseconds before timing out or retry gateway transactions.*/
    const int timeout_gateway_ms_;
    /** This many milliseconds before timing out or retry comport transactions.*/
    const int timeout_comport_ms_;

    /** Do up to this many comport retries. */
    const int retries_com_;
    /** Do up to this many gateway retries. */
    const int retries_gateway_;

    // 2620 channel usage for this app:
    /** Pulse width measurement.*/
    const int s2620_channel_width_;
    /** Frequency counter. */
    const int s2620_channel_freq_;
    /** Pulse width modulated output. */
    const int s2620_channel_pwm_;
    /** Incremental encoder input. */
    const int s2620_channel_encoder_;

    // Comport usage for this app.  With two null-modem cables, we can loop back two ports into two other ports:
    /** Transmit A. */
    const u8 com_src_a_;
    /** Receive A. */
    const u8 com_dest_a_;
    /** Baudrate for A. */
    const u16 com_baud_a_;

    /** Transmit B. */
    const u8 com_src_b_;
    /** Receive B. */
    const u8 com_dest_b_;
    /** Baudrate for B. */
    const u16 com_baud_b_;

    /** Ignore the comport REJ flag. */
    const int com_reject_ignore_;
    /** Treat comport REJ flag as an error. */
    const int com_reject_evaluate_;

    // PUBLIC STORAGE ///////////////////////////////////////////////////////////////
    /** Number of times through the control loop so far. */
    int   iters_ctrl_loop_;

    /** Number of detected iom's. */
    u16   num_iom_boards_;
    /** Detected iom types. */
    u16   iom_types_[16];
    /** Iom status info. */
    u8    iom_status_[16];

    /** Number of dac channels (applies to 2608 only). */
    u8    s2608_num_aouts_at_iom_[16];

    // Input data from the i/o system.
    /** IOM port Link status. */
    u16   iom_link_flags_;
    /** Interlock power status. */
    u8    interlock_flags_;
    /** Digital input states (48 channels). */
    u8    num_digital_in_states_[6];
    /** Analog input voltages. */
    DOUBLE  analog_in_voltages_[16];

    // Output data to the i/o system.
    /** Relay states. */
    u8    num_relay_states_;
    /** Digital output states (48 channels). */
    u8    num_digital_out_states_[6];
    /** Analog output voltages. */
    DOUBLE  analog_out_voltages_[MAX_NUM_AOUTS];
    /** Counter data. */
    u32   counter_counts_[4];
    /** Counter timestamps. */
    u16   counter_timestamp_[4];
  };

  /** This class provides a simple interface to connect
   * to a Sensoray board, read encoder positions and send
   * analog force/torque commands */
  class CSensoray3DofIO
  {
  public:
    /** Default constructor : Does nothing */
    CSensoray3DofIO() : s_ds_(), totalSent(0), peek_character(-1) {}

    /** Default destructor : Does nothing */
    ~CSensoray3DofIO() {}

    /** Get data */
    SSensoray3DofIO& getData()
    { return s_ds_; }

    //Non-static functions
    void ShowErrorInfo( u32 gwerr, u8 *iom_status_ );
    u32 ComError( u32 gwerr, const char *fname, int evalComReject );
    void sched_io( void* x );
    int SerialInit( u8 ComSrc, u8 ComDst, u16 BaudRate );
    int SerialIo( u8 ComSrc, u8 ComDst );
    int io_control_loop( void );

    // Static fn. FORWARD REFERENCES ////////////////////////////////////////////////////////////

    int  io_exec( void* x );
    void io_control_main( void );
    int  DetectAllIoms( void );

    void* CreateTransaction( HBD hbd );
    void kbopen( void );
    void kbclose( void );
    int kbhit( void );
    int kbread( void );

  private:
    SSensoray3DofIO s_ds_;

    int  totalSent;

    int        peek_character;
    struct termios initial_settings;
    struct termios new_settings;
  };

} /* namespace sensoray */
#endif /* CSENSORAY3DOFIO_HPP_ */