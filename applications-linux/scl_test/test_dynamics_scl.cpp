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
/* \file test_dynamics_scl.hpp
 *
 *  Created on: Oct 20, 2013
 *
 *  Copyright (C) 2013
 *
 *  Author: Samir Menon <smenon@stanford.edu>
 */

#include "test_dynamics.hpp"

#include <sutil/CSystemClock.hpp>

#include <scl/DataTypes.hpp>
#include <scl/Singletons.hpp>
#include <scl/robot/DbRegisterFunctions.hpp>
#include <scl/parser/sclparser/CSclParser.hpp>
#include <scl/util/DatabaseUtils.hpp>

//Scl Dynamics
#include <scl/dynamics/scl/CDynamicsScl.hpp>

#include <scl/dynamics/analytic/CDynamicsAnalyticRPP.hpp>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <cmath>
#include <stdio.h>
#include <math.h>

#include <Eigen/Dense>

using namespace scl;
using namespace std;

namespace scl_test
{
  /** Tests the performance of analytical dynamics implementations in scl. */
    void test_dynamics_scl_vs_analytic_rpp(int id)
    {
      scl::CDynamicsScl dynamics;
      scl::sUInt r_id=0;
      bool flag;

      const double test_precision = 0.00001;

      try
      {
        //0. Create vars
        long long i; //Counters
        long long imax; //Counter limits
        sClock t1,t2; //Clocks: pre and post

        //Test database
        scl::SDatabase * db = scl::CDatabase::getData();
        if(S_NULL==db)
        { throw(std::runtime_error("Database not initialized."));  }
        else
        { std::cout<<"\nTest Result ("<<r_id++<<")  Initialized database"<<std::flush;  }

        db->dir_specs_ = scl::CDatabase::getData()->cwd_ + std::string("../../specs/");

        //0. Parse the file for robots
        std::string tmp_infile;
        tmp_infile = db->dir_specs_ + "Bot-RPP/Bot-RPPCfg.xml";
        std::cout<<"\nTest Result ("<<r_id++<<")  Opening file : "<<tmp_infile;

        scl_parser::CSclParser tmp_lparser;

        //1 Create robot from the file specification (And register it with the db)
        std::string robot_name = "rppbot";
        flag = scl_registry::parseRobot(tmp_infile, robot_name, &tmp_lparser);
        if(false == flag)
        { throw(std::runtime_error("Could not register robot with the database"));  }
        else
        {
          std::cout<<"\nTest Result ("<<r_id++<<")  Created a robot "
              <<robot_name<<" on the pile"<<std::flush;
        }

        // Check the robot was parsed.
        scl::SRobotParsedData *rob_ds = db->s_parser_.robots_.at(robot_name);
        if(NULL == rob_ds)
        { throw(std::runtime_error("Could not find registered robot in the database"));  }

        // IF the robot wasn't sorted, issue a warning and set present order as sorted
        std::vector<std::string> tmp_sort_order;
        flag = rob_ds->robot_br_rep_.sort_get_order(tmp_sort_order);
        if(false == flag)
        {
          std::cout<<"\nWARNING : Robot branching representation is not sorted by default. Sorting. Robot = "<<rob_ds->name_;

          // Get the present node ordering.
          sutil::CMappedTree<std::string, SRigidBody>::const_iterator it,ite;
          for(it = rob_ds->robot_br_rep_.begin(), ite = rob_ds->robot_br_rep_.end();
                    it!=ite; ++it)
          { tmp_sort_order.push_back(it->name_); }

          // Sort it.
          flag = rob_ds->robot_br_rep_.sort(tmp_sort_order);
          if(false == flag)
          { throw(std::runtime_error("Could not sort unsorted robot branching representation."));  }
        }

  #ifdef DEBUG
        std::cout<<"\nPrinting parsed robot "<<rob_ds->name_;
        scl_util::printRobotLinkTree( *(rob_ds->robot_br_rep_.getRootNode()), 0);
  #endif

        //*********** Create the dynamics computational object *************
        // Initialize the dynamics computational object
        flag = dynamics.init(*rob_ds);
        if (false==flag) { throw(std::runtime_error("Failed to initialize scl dynamics."));  }
        else { std::cout<<"\nTest Result ("<<r_id++<<")  Initialized scl dynamics for the robot.";  }

        //*********** Create and intialize the analytic dynamics computational object *************
        scl::CDynamicsAnalyticRPP dyn_anlyt;
        flag = dyn_anlyt.init(*rob_ds);
        if (false==flag) { throw(std::runtime_error("Failed to initialize analytic dynamics."));  }

        SRobotIOData * io_ds;
        io_ds = scl::CDatabase::getData()->s_io_.io_data_.at(robot_name);
        if(S_NULL == io_ds)
        { throw(std::runtime_error("Could not find the robot's I/O data structure in the database"));  }

        //******************* Now test the actual implementation ******************
        io_ds->sensors_.q_.setZero(3);
        io_ds->sensors_.dq_.setZero(3);
        io_ds->sensors_.ddq_.setZero(3);

        // *********************************************************************************************************
        //                              Set up the robot's dynamics data struct
        // *********************************************************************************************************
        // These are used to compute the full dynamics model later in the code..
        scl::SGcModel rob_gc_model;
        flag = rob_gc_model.init(*rob_ds);
        if(false == flag)
        { throw(std::runtime_error("Could not create a dynamic object for the robot"));  }

        // *********************************************************************************************************
        //                                     Test Transformation Matrix
        // *********************************************************************************************************
        // Set up variables.
        Eigen::Affine3d Tscl, Tanlyt;
        std::string link_name;

        // Note q is zero here.
        sutil::CMappedTree<std::string, SRigidBodyDyn>::iterator it,ite;
        for(it = rob_gc_model.link_ds_.begin(), ite = rob_gc_model.link_ds_.end();
            it!=ite; ++it)
        {
          // Test : Link 0
          link_name = it->name_;

          // Skip the root node (all matrices are zero).
          if(it->link_ds_->is_root_) { continue; }

          flag = dynamics.calculateTransformationMatrixForLink(*it,io_ds->sensors_.q_);
          if (false==flag) { throw(std::runtime_error("Failed to compute scl link transformation matrix."));  }
          Tscl = it->T_lnk_;

          flag = dyn_anlyt.calculateTransformationMatrix(io_ds->sensors_.q_, it->link_ds_->link_id_,
              it->link_ds_->link_id_-1/**NOTE: Trf to parent, not root*/, Tanlyt);
          if (false==flag) {
            throw(std::runtime_error(std::string("Failed to compute analytic transformation matrix at: ") + link_name));
          }

          for(int i=0; i<4 && flag; i++)
            for(int j=0; j<4 && flag; j++)
            { flag = flag && (fabs(Tscl.matrix()(i,j) - Tanlyt.matrix()(i,j))<test_precision);  }

          if (false==flag)
          {
            std::cout<<"\nScl transform Org->"<<link_name<<":\n"<<Tscl.matrix();
            std::cout<<"\nAnalytic transform Org->"<<link_name<<":\n"<<Tanlyt.matrix();
            throw(std::runtime_error("Scl and analytic transformation matrices don't match."));
          }
          else { std::cout<<"\nTest Result ("<<r_id++<<")  Analytic and scl transformations match for zero position : "<<link_name;  }

#ifdef DEBUG
          std::cout<<"\nScl transform Org->"<<link_name<<":\n"<<Tscl.matrix();
          std::cout<<"\nAnalytic transform Org->"<<link_name<<":\n"<<Tanlyt.matrix();
#endif
        }

        // *********************************************************************************************************
        //                                         Test Com Jacobians
        // *********************************************************************************************************
        // Set up variables.
        Eigen::MatrixXd Jcom_scl, Jcom_anlyt;
        Eigen::VectorXd pos;
        pos.setZero(3);
        std::string link_name;

        sutil::CMappedTree<std::string, SRigidBody>::const_iterator it,ite;
        for(it = rob_ds->robot_br_rep_.begin(), ite = rob_ds->robot_br_rep_.end();
            it!=ite; ++it)
        {
          // Test : Link 0
          link_name = it->name_;

          // Skip the root node (all matrices are zero).
          if(it->is_root_) { continue; }

          pos = it->com_;
          flag = dynamics.calculateJacobian(dynamics.getIdForLink(link_name),pos,Jcom_scl);
          if (false==flag) { throw(std::runtime_error("Failed to compute scl com Jacobian."));  }

          flag = dyn_anlyt.computeJcom(io_ds->sensors_.q_, dyn_anlyt.getIdForLink(link_name), Jcom_anlyt);
          if (false==flag) { throw(std::runtime_error("Failed to compute analytic com Jacobian."));  }

          for(int i=0; i<3; i++)
            for(int j=0; j<3; j++)
            { flag = flag && fabs(Jcom_scl(i,j) - Jcom_anlyt(i,j)) < test_precision; }

          if (false==flag)
          {
            std::cout<<"\nScl Jcom_"<<link_name<<":\n"<<Jcom_scl;
            std::cout<<"\nAnalytic Jcom_"<<link_name<<":\n"<<Jcom_anlyt;
            throw(std::runtime_error("Scl and analytic Jacobians don't match."));
          }
          else { std::cout<<"\nTest Result ("<<r_id++<<")  Analytic and scl com Jacobians match for zero position : "<<link_name;  }

  #ifdef DEBUG
          std::cout<<"\nScl Jcom_"<<link_name<<":\n"<<Jcom_scl;
          std::cout<<"\nAnalytic Jcom_"<<link_name<<":\n"<<Jcom_anlyt;
  #endif
        }

        // ********************************************************************************************************
        std::cout<<"\nTest #"<<id<<" : Succeeded.";
      }
      catch (std::exception& ee)
      {
        std::cout<<"\nTest Result ("<<r_id++<<") : "<<ee.what();
        std::cout<<"\nTest #"<<id<<" : Failed.";
      }
    }
}

