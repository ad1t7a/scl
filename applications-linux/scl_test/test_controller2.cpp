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
/* \file test_controller2.cpp
 *
 *  Created on: Apr 02, 2017
 *
 *  Copyright (C) 2017
 *
 *  Author: Samir Menon <smenon@stanford.edu>
 */

#include "test_robot_controller.hpp"

// Just to make sure we don't have conflicts..
#include <scl/scl.hpp>
// We're testing the second version of the controller
#include <scl/control/tasks/AllHeaders.hpp>

namespace scl_test
{
  /**
   * Tests the performance of the task controller
   * on the given robot specification:
   */
  void test_controller2(int id)
  {
    scl::sUInt r_id=0;
    bool flag=true;

    try
    {
      //We'll just go with the Puma for now..
      scl::CParserScl p;
      scl::SRobotParsed rds;
      scl::SRobotIO rio;
      scl::SGcModel rgcm;
      scl::CDynamicsScl dyn_scl;

      // ********************** ROBOT PARSER TESTING **********************
      flag = flag && p.readRobotFromFile("../../specs/Puma/PumaCfg.xml", "../../specs/", "PumaBot", rds);
      flag = flag && rgcm.init(rds);
      flag = flag && rio.init(rds);
      flag = flag && dyn_scl.init(rds);
      //Test code.
      if(false==flag) { throw(std::runtime_error("Could not parse robot from file and init data/dynamics."));  }
      else { std::cout<<"\nTest Result ("<<r_id++<<")  Parsed robot from file and init data/dynamics."<<std::flush;  }

      // ********************** CONTROL TASK DS TESTING **********************
      // Let's test a control task..
      scl::tasks::STaskOpPos t_op_ds;
      sutil::CMappedList<std::string, std::string> params;

      // Initialize the task parameters
      std::string *tt;
      tt = params.create(std::string("name"),std::string("hand"));
      // As a sanity check, we'll just test the first key. We'll assume the others work..
      if(NULL == tt)
      { throw(std::runtime_error("Could not create name:hand key in mapped list."));  }
      else { std::cout<<"\nTest Result ("<<r_id++<<") Created key value pair name:"<<*tt<<". Testing recall from list for val: "<<*params.at_const("name")<<std::flush;  }
      params.create(std::string("type"),std::string("TaskOpPos"));
      params.create(std::string("priority"),std::string("0"));
      params.create(std::string("task_dof"),std::string("3"));
      params.create(std::string("kp"),std::string("10"));
      params.create(std::string("kv"),std::string("3"));
      params.create(std::string("ka"),std::string("0"));
      params.create(std::string("ki"),std::string("0"));
      params.create(std::string("ftask_max"),std::string("10"));
      params.create(std::string("ftask_min"),std::string("-10"));
      params.create(std::string("parent_link"),std::string("end-effector"));
      params.create(std::string("pos_in_parent"),std::string("0.01 0.00 0.00"));
      params.create(std::string("flag_compute_op_gravity"),std::string("true"));
      params.create(std::string("flag_compute_op_cc_forces"),std::string("false"));
      params.create(std::string("flag_compute_op_inertia"),std::string("true"));

      // Initialize the task
      flag = t_op_ds.init(params,&rds);
      //Test code.
      if(false==flag) { throw(std::runtime_error("Could not init op task data from the parsed params."));  }
      else { std::cout<<"\nTest Result ("<<r_id++<<")  Init op task data from the parsed params."<<std::flush;  }

      // Copy the task
      scl::tasks::STaskOpPos *t_op_ds2 = new scl::tasks::STaskOpPos(t_op_ds);
      if( t_op_ds2->J_.rows() != t_op_ds.J_.rows() || t_op_ds2->J_.cols() != t_op_ds.J_.cols())
      { throw(std::runtime_error("Could not init op task data from the parsed params."));  }
      else { std::cout<<"\nTest Result ("<<r_id++<<")  Copy constructor worked for the op pos task."<<std::flush;  }
      delete t_op_ds2;

//      // NOTE TODO : FIX THIS LATER WHEN JSON STUFF WORKS!!
//      // Serialize / deserialize the task
//      std::string str_json;
//      flag = scl::serializeToJSONString(t_op_ds,str_json,true);
//      if(false==flag) { throw(std::runtime_error("Could not serialize op task data into json."));  }
//      else { std::cout<<"\nTest Result ("<<r_id++<<")  Serialized op task data into json."<<std::flush;  }

      // ********************** CONTROL TASK TESTING **********************
      scl::tasks::CTaskOpPos t_op;

      flag = t_op.init(params, rds);
      if(false==flag) { throw(std::runtime_error("Could not init op task object from the parsed params."));  }
      else { std::cout<<"\nTest Result ("<<r_id++<<")  Init op task object using the parsed params."<<std::flush;  }

      flag = t_op.init(t_op_ds);
      t_op_ds2 = dynamic_cast<scl::tasks::STaskOpPos *>(t_op.getData());
      if( false == flag || NULL == t_op_ds2 )
      { throw(std::runtime_error("Could not init op task object using an existing task data struct."));  }

      if(t_op_ds2->J_.rows() != t_op_ds.J_.rows() || t_op_ds2->J_.cols() != t_op_ds.J_.cols())
      { throw(std::runtime_error("Could not init op task object using an existing task data struct."));  }
      else { std::cout<<"\nTest Result ("<<r_id++<<")  Init op task object using a pre-initialized data struct."<<std::flush;  }

      flag = t_op.computeModel(rio.sensors_,rgcm, dyn_scl);
      if(false == flag)
      { throw(std::runtime_error("Could not compute task model."));  }
      else { std::cout<<"\nTest Result ("<<r_id++<<")  Computed task model."<<std::flush;  }

      flag = t_op.computeControl(rio.sensors_,rgcm, dyn_scl);
      if(false == flag)
      { throw(std::runtime_error("Could not compute task control."));  }
      else { std::cout<<"\nTest Result ("<<r_id++<<")  Computed task control."<<std::flush;  }

      std::cout<<"\nTest #"<<id<<" (Task Controller2) : Succeeded.";
    }
    catch (std::exception& ee)
    {
      std::cout<<"\nTest Result ("<<r_id++<<") : ERROR : "<<ee.what();
      std::cout<<"\nTest #"<<id<<" (Task Controller2) : Failed.";
    }
  }

}
