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
/* \file CTaskController.cpp
 *
 *  Created on: Jul 21, 2010
 *
 *  Copyright (C) 2010
 *
 *  Author: Samir Menon <smenon@stanford.edu>
 */

#include "CTaskController.hpp"

#include <sutil/CRegisteredDynamicTypes.hpp>

#include <iostream>
#include <stdexcept>


namespace scl
{
  /**********************************************
   *               INITIALIZATION
   * ********************************************/
  CTaskController::CTaskController() : CControllerBase()
  {
    active_task_ = S_NULL;
    data_= S_NULL;
    dynamics_ = S_NULL;
    task_count_ = 0;
  }

  sBool CTaskController::init(SControllerBase* arg_data,
      scl::CDynamicsBase* arg_dynamics)
  {
    try
    {
      //Reset the computational object (remove all the associated data).
      reset();

      if(S_NULL==arg_data)
      { throw(std::runtime_error("NULL data structure passed."));  }
      if(false==arg_data->has_been_init_)
      { throw(std::runtime_error("Uninitialized data structure passed."));  }
      //This ensures that the passed data was properly initialized for task controllers.
      data_ = dynamic_cast<STaskController*>(arg_data);

      if(NULL==arg_dynamics)
      { throw(std::runtime_error("NULL dynamics object passed."));  }
      if(false==arg_dynamics->hasBeenInit())
      { throw(std::runtime_error("Uninitialized dynamics object passed."));  }
      dynamics_ = arg_dynamics;

      has_been_init_ = true;

      //Point the servo computational object to the data struct
      //This also initializes the servo data
      sBool flag;
      flag = servo_.init(data_->robot_name_, &(data_->servo_) );
      if(false == flag)
      { throw(std::runtime_error("Couldn't initialize the servo object.")); }

      //Add CTask objects for all the initialized tasks
      //Read Luis Sentis' thesis for a theoretical description of how multiple task levels work.
      std::vector<std::vector<STaskBase**> >::iterator it,ite;
      for(it = data_->tasks_.mlvec_.begin(), ite = data_->tasks_.mlvec_.end();
          it!=ite; ++it)
      {//--------------------For each level--------------------
        std::vector<STaskBase**>::iterator it2, ite2;
        for(it2 = (*it).begin(), ite2 = (*it).end();
            it2!=ite2; ++it2)
        {//----------For every task in the level----------
          CTaskBase* tmp_task;
          STaskBase* tmp_task_ds = *(*it2);
          if(S_NULL == tmp_task_ds)
          { throw(std::runtime_error("Passed SControllerBase object has a NULL task pointer."));  }

          std::string tmp_type = "C" + tmp_task_ds->type_task_; //Computational object of this type.
          int tmp_task_pri = tmp_task_ds->priority_;
              //data_->tasks_.getPriorityLevel(&tmp_task_ds);

          void* obj = S_NULL;
          flag = sutil::CRegisteredDynamicTypes<std::string>::getObjectForType(tmp_type,obj);
          if(false == flag)
          {
            std::stringstream ss;
            ss<<"Dynamic controller type not initialized for task " <<tmp_task_ds->name_<<" of type "
                << tmp_type <<" at level "<<tmp_task_pri;
            std::string err;
            err = ss.str();
            throw(std::runtime_error(err.c_str()));
          }
          tmp_task = reinterpret_cast<CTaskBase*>(obj);

          //Now initialize the task
          flag = tmp_task->init(tmp_task_ds, arg_dynamics);
          if(false == flag)
          {
            std::stringstream ss;
            ss<<"Could not initialize task " <<tmp_task_ds->name_<<" of type "
                << tmp_type <<" at level "<<tmp_task_pri;
            std::string err;
            err = ss.str();
            throw(std::runtime_error(err.c_str()));
          }

          flag = addTask(tmp_task_ds->name_,tmp_task,tmp_task_pri);
          if(false == flag)
          {
            std::stringstream ss;
            ss<<"Could not add task " <<tmp_task_ds->name_<<" of type "
                << tmp_type <<" at level "<<tmp_task_pri;
            std::string err;
            err = ss.str();
            throw(std::runtime_error(err.c_str()));
          }
        }
      }

      has_been_init_ = true; //Can't really do anything with the data structure yet.

    }
    catch(std::exception& e)
    {
      has_been_init_ = false;
      std::cout<<"\nCTaskController::init() Error : "<<e.what();
    }
    return has_been_init_;
  }

  sBool CTaskController::reset()
  {
    //Remove all data references from this task controller.
    //It will require re-initialization after this.

    data_ = S_NULL;
    dynamics_ = S_NULL;

    servo_.reset();

    /**
     * NOTE : This is a rather complicated thing to do.
     *
     * Resetting the tasks can be done in two ways:
     * 1. Simply forget everything about every task. Then there
     * isn't enough information to reconstruct the tasks after that
     * since the CTaskBase* pointers point to some subclass which is lost.
     *
     * 2. Individually reset each task. In this case, the reset tasks
     * need to be seeded with their data structures again which is
     * just as hard to do as (1).
     *
     * For now we'll just do (1)
     */
    tasks_.clear();
    task_count_ = 0;

    active_task_ = S_NULL;

    return true;
  }

  sBool CTaskController::addTask(const std::string &arg_task_name,
      CTaskBase* arg_task, sUInt arg_level)
  {
    sBool flag;
    try
    {
      if(NULL==data_)
      { throw(std::runtime_error("CTaskController not initialized. Can't add task."));  }

      if(NULL==arg_task)
      { throw(std::runtime_error("Passed an empty task. Can't do anything with it."));  }

      if(arg_level < 0)
      { throw(std::runtime_error("Can't add a task at a level less than 0"));  }

      //Initialize the task's data structure.
      flag = arg_task->hasBeenInit();
      if(false == flag) { throw(std::runtime_error("Passed an un-initialized task."));  }

      flag = tasks_.create(arg_task_name, arg_task, arg_level);
      if(false == flag) { throw(std::runtime_error("Could not create a task computational object."));  }

      //Works best for only one task
      if(0 == task_count_)
      { active_task_ = arg_task;  }

      task_count_++;

      return true;
    }
    catch(std::exception& e)
    { std::cout<<"\nCTaskController::addTask() : Failed. "<<e.what(); }
    return false;
  }

  sBool CTaskController::removeTask(const std::string &arg_task_name)
  {
    bool flag;
    try
    {
      CTaskBase** tmp = tasks_.at(arg_task_name);
      if(S_NULL == tmp)
      { throw(std::runtime_error("Could not find task to delete."));  }

      flag = tasks_.erase(arg_task_name);
      if(false == flag)
      { throw(std::runtime_error("Could not delete a task computational object."));  }

      delete *tmp; tmp = S_NULL;

      task_count_--;
      if(1== task_count_)
      {
        active_task_ = *(tasks_.begin());
      }
    }
    catch(std::exception& e)
    {
      std::cout<<"\nCTaskController::removeTask() : Failed. "<<e.what();
      return false;
    }
    return true;
  }

  /** Returns the task by this name */
  CTaskBase * CTaskController::getTask(const std::string& arg_name)
  {
    try
    {
      CTaskBase ** ret;
      ret = tasks_.at(arg_name);
      if(S_NULL == ret)
      { throw(std::runtime_error("Task not found in the pile"));  }

      if(S_NULL == *ret)
      { throw(std::runtime_error("NULL task found in the pile"));  }

      return *ret;
    }
    catch(std::exception& e)
    { std::cout<<"\nCTaskController::getTask() : Failed. "<<e.what(); }
    return S_NULL;
  }

  sUInt CTaskController::getNumTasks(const std::string& arg_type) const
  {
    sutil::CMappedMultiLevelList<std::string, STaskBase*>::const_iterator it,ite;

    //Loop over all the tasks.
    sUInt ctr = 0;
    for(it = data_->tasks_.begin(), ite = data_->tasks_.end();
        it!=ite;++it)
    {
      STaskBase* t = *it;
      if(arg_type == t->type_task_)
      { ctr++;  }
    }

    return ctr;
  }

  /**********************************************
   *               COMPUTATION
   ***********************************************/
  sBool CTaskController::computeControlForces()
  {
    //Compute the task torques
    sBool flag=true;
    if(1==task_count_)
    {
      flag = active_task_->computeServo(&(data_->io_data_->sensors_));
      flag = flag && servo_.computeControlForces();

      STaskBase* t = active_task_->getTaskData();
      data_->io_data_->actuators_.force_gc_commanded_ = t->force_gc_;
    }
    else
    {
      sutil::CMappedMultiLevelList<std::basic_string<char>, scl::CTaskBase*>::iterator it, ite;
      for(it = tasks_.begin(), ite = tasks_.end(); it!=ite; ++it)
      {
        CTaskBase* task = *it;
        flag = flag && task->computeServo(&(data_->io_data_->sensors_));
      }

      //Compute the command torques by filtering the
      //various tasks through their range spaces.
      flag = flag && servo_.computeControlForces();

      data_->io_data_->actuators_.force_gc_commanded_ = data_->servo_.force_gc_;
    }

    return flag;
  }

  sBool CTaskController::computeDynamics()
  {
    sBool flag=true;

    //Update the joint space dynamic matrices
    flag = dynamics_->updateModelMatrices(&(data_->io_data_->sensors_), &(data_->gc_model_));

    // Compute the task space dynamics
    if(0==task_count_)
    { return false; }
    if(1==task_count_)
    { flag = flag && active_task_->computeModel();  }
    else
    {
      sutil::CMappedMultiLevelList<std::basic_string<char>, scl::CTaskBase*>::iterator it, ite;
      for(it = tasks_.begin(), ite = tasks_.end(); it!=ite; ++it)
      {
        CTaskBase* task = *it;
        flag = flag && task->computeModel();
      }
    }

    //Compute the range spaces for all the tasks.
    flag = flag && computeRangeSpaces();

    return flag;
  }

  sBool CTaskController::computeRangeSpaces()
  {
    sUInt dof = data_->io_data_->dof_;
    if(1==task_count_)
    {
      STaskBase* tmp = active_task_->getTaskData();
      tmp->range_space_.setIdentity(dof, dof);
      return true;
    }
    else
    {
      Eigen::MatrixXd null_space;//The null space within which each successive level operates
      null_space.setIdentity(dof, dof);//Initially no part of the gen-coords is used up

      sUInt levels = tasks_.getNumPriorityLevels();
      for(sUInt i=0;i<levels;++i)
      {//We directly write into the task data structures.
        std::vector<STaskBase**>* taskvec = data_->tasks_.getSinglePriorityLevel(i);
        std::vector<STaskBase**>::iterator it,ite;

        //This level will use us some of the gen-coord space. So
        //the next level will operate within this level's null space
        Eigen::MatrixXd lvl_null_space;
        lvl_null_space.setIdentity(dof, dof);

        for(it = taskvec->begin(),ite = taskvec->end();it!=ite;++it)
        {
          STaskBase *task_ds = *(*it);
          task_ds->range_space_ = null_space;//Set this task's range space to the higher level's null_space
          lvl_null_space *= task_ds->null_space_;//Reduce this level's null space
        }
        //The next level's range space is filtered through this level's null space
        null_space *= lvl_null_space;
      }

      return true;
    }
    return false;
  }
}
