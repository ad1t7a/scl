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
/* \file CChaiHaptics.cpp
 *
 *  Created on: Sep 3, 2012
 *
 *  Copyright (C) 2012
 *
 *  Author: Samir Menon <smenon@stanford.edu>
 */

#include <scl/haptics/chai/CChaiHaptics.hpp>
#include <chai3d.h>

#include <stdio.h>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace scl
{
  CChaiHaptics::~CChaiHaptics()
  {
    closeConnectionToDevices();
    if(NULL!=haptics_handler_)
    { delete haptics_handler_;  }
  }


  scl::sInt CChaiHaptics::connectToDevices()
  {

    try
    {
      // Create a haptic device handler :
      // NOTE : Chai apparently connects to the new devices within the
      //        constructor (Could lead to weird error states!).
      haptics_handler_ = new cHapticDeviceHandler();

      if (NULL == haptics_handler_)
      { throw(std::runtime_error("Could not create haptic handler (out of memory?)")); }
      else
      {
        std::cout<<"\nCChaiHaptics::connectToDevices() : Searched and connected to ["
            <<haptics_handler_->getNumDevices() <<"] devices";
      }

      // ***********************************************************
      //                     CONNECT TO DEVICES
      // ***********************************************************
      for(unsigned int i=0;i<haptics_handler_->getNumDevices();++i)
      {// Now connect to all the haptic devices.
        cGenericHapticDevice* tmp_haptic_device = NULL;

        // 1. Get a handle to the haptic device
        int tmp = haptics_handler_->getDevice(tmp_haptic_device, i);
        if (0 != tmp)
        {
          std::stringstream ss; ss<<"Could not register haptic device: "<<i;
          throw(std::runtime_error(ss.str()));
        }
        else
        { std::cout << "\nRegistered haptic device: "<< i << std::flush; }

        // 2. Open a connection to the haptic device.
        tmp = tmp_haptic_device->open();
        if (0 != tmp)
        {
          std::stringstream ss; ss<<"Could not connect to haptic device: "<<i;
          throw(std::runtime_error(ss.str()));
        }
        else
        { std::cout << "\nConnected to haptic device: "<< i << std::flush; }

        //Add the haptic device pointer to the vector
        haptic_devices_.push_back(tmp_haptic_device);
      }//End of loop over haptic devices
    }
    catch (std::exception& e)
    {
      std::cerr << "\nCChaiHaptics::connectToDevices() :" << e.what();
      return false;
    }
    return true;
  }

  bool CChaiHaptics::getHapticDevicePositions(
      std::vector<Eigen::VectorXd>& ret_pos_vec)
  {
    if(ret_pos_vec.size() > haptics_handler_->getNumDevices())
    {
      std::cout<<"\n The passed vector has more elements than the number of connected devices";
      return false;
    }

    // ***********************************************************
    //                     READ DEVICE STATE
    // ***********************************************************
    std::vector<cGenericHapticDevice*>::iterator it,ite;
    std::vector<Eigen::VectorXd>::iterator itv, itve;
    int i=0;
    for(it = haptic_devices_.begin(), ite = haptic_devices_.end(),
        itv = ret_pos_vec.begin(), itve = ret_pos_vec.end();
        it!=ite && itv!=itve; ++it, ++itv)
    {// Now read the state from the devices.
      cVector3d tmpv;
      int tmp =(*it)->getPosition(tmpv); //Read position into tmpv
      *itv = tmpv;                       //Set the positions in the vector
      if (0 != tmp)
      { std::cout<<"\nWARNING : Could not read position of haptic device: "<<i; }
      i++;
    }
    return true;
  }

  bool CChaiHaptics::closeConnectionToDevices()
  {
    bool flag = true;
    // ***********************************************************
    //                        CLOSE DEVICES
    // ***********************************************************
    int i=0;
    std::vector<cGenericHapticDevice*>::iterator it,ite;
    for(it = haptic_devices_.begin(), ite = haptic_devices_.end();
        it!=ite; ++it)
    {// Now read the state from the devices.
      int tmp = (*it)->close();
      if (0 != tmp)
      {
        std::cout<<"\nWARNING: Could not close connection to haptic device: "<<i;
        flag = false;
      }
      else
      { std::cout<<"\nClosed connection to haptic device: "<<i; }
      i++;
    }
    return flag;
  }

} /* namespace scl */