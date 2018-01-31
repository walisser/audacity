/**********************************************************************

  Audacity: A Digital Audio Editor

  ODTask.cpp

  Created by Michael Chinen (mchinen)
  Audacity(R) is copyright (c) 1999-2008 Audacity Team.
  License: GPL v2.  See License.txt.

******************************************************************//**

\class ODTask
\brief ODTask is an abstract class that outlines the methods that will be used to
support On-Demand background loading of files.  These ODTasks are generally meant to be run
in a background thread.

*//*******************************************************************/


#include "ODTask.h"
//--#include "ODManager.h"
#include "../WaveTrack.h"
//--#include "../Project.h"
//--#include "../UndoManager.h"
//temporarilly commented out till it is added to all projects
//#include "../Profiler.h"

class AudacityProject;

//--DEFINE_EVENT_TYPE(EVT_ODTASK_COMPLETE)

/// Constructs an ODTask
ODTask::ODTask()
: mDemandSample(0)
{

   static int sTaskNumber=0;
   mPercentComplete=0;
   mDoingTask=false;
   mTerminate = false;
   mNeedsODUpdate=false;
   mIsRunning = false;

   mTaskNumber=sTaskNumber++;
}

//outside code must ensure this task is not scheduled again.
void ODTask::TerminateAndBlock()
{
   //one mutex pair for the value of mTerminate
   mTerminateMutex.lock();
   mTerminate=true;
   //release all data the derived class may have allocated
   mTerminateMutex.unlock();

   //and one mutex pair for the exit of the function
   mBlockUntilTerminateMutex.lock();
//TODO lock mTerminate?
   mBlockUntilTerminateMutex.unlock();

   //wait till we are out of doSome() to terminate.
   Terminate();
}

///Do a modular part of the task.  For example, if the task is to load the entire file, load one BlockFile.
///Relies on DoSomeInternal(), which is the subclasses must implement.
///@param amountWork the percent amount of the total job to do.  1.0 represents the entire job.  the default of 0.0
/// will do the smallest unit of work possible
void ODTask::DoSome(float amountWork)
{
   SetIsRunning(true);
   mBlockUntilTerminateMutex.lock();

//   wxPrintf("%s %i subtask starting on NEW thread with priority\n", GetTaskName(),GetTaskNumber());

   mDoingTask=mTaskStarted=true;

   float workUntil = amountWork+PercentComplete();



   //check periodically to see if we should exit.
   mTerminateMutex.lock();
   if(mTerminate)
   {
      mTerminateMutex.unlock();
      SetIsRunning(false);
      mBlockUntilTerminateMutex.unlock();
      return;
   }
   mTerminateMutex.unlock();

   Update();


   if(UsesCustomWorkUntilPercentage())
      workUntil = ComputeNextWorkUntilPercentageComplete();

   if(workUntil<PercentComplete())
      workUntil = PercentComplete();

   //Do Some of the task.

   mTerminateMutex.lock();
   while(PercentComplete() < workUntil && PercentComplete() < 1.0 && !mTerminate)
   {
      //--wxThread::This()->Yield();
      QThread::yieldCurrentThread();
      //release within the loop so we can cut the number of iterations short

      DoSomeInternal(); //keep the terminate mutex on so we don't remo
      mTerminateMutex.unlock();
      //check to see if ondemand has been called
      if(GetNeedsODUpdate() && PercentComplete() < 1.0)
         ODUpdate();


      //But add the mutex lock back before we check the value again.
      mTerminateMutex.lock();
   }
   mTerminateMutex.unlock();
   mDoingTask=false;

   mTerminateMutex.lock();
   //if it is not done, put it back onto the ODManager queue.
   if(PercentComplete() < 1.0&& !mTerminate)
   {
      /**--
      ODManager::Instance()->AddTask(this);

      //we did a bit of progress - we should allow a resave.
      ODLocker locker{ &AudacityProject::AllProjectDeleteMutex() };
      for(unsigned i=0; i<gAudacityProjects.size(); i++)
      {
         if(IsTaskAssociatedWithProject(gAudacityProjects[i].get()))
         {
            //mark the changes so that the project can be resaved.
            gAudacityProjects[i]->GetUndoManager()->SetODChangesFlag();
            break;
         }
      }
      --**/


//      wxPrintf("%s %i is %f done\n", GetTaskName(),GetTaskNumber(),PercentComplete());
   }
   else
   {
      //for profiling, uncomment and look in audacity.app/exe's folder for AudacityProfile.txt
      //static int tempLog =0;
      //if(++tempLog % 5==0)
         //END_TASK_PROFILING("On Demand Drag and Drop 5 80 mb files into audacity, 5 wavs per task");
      //END_TASK_PROFILING("On Demand open an 80 mb wav stereo file");

      /**--
      wxCommandEvent event( EVT_ODTASK_COMPLETE );

      ODLocker locker{ &AudacityProject::AllProjectDeleteMutex() };
      for(unsigned i=0; i<gAudacityProjects.size(); i++)
      {
         if(IsTaskAssociatedWithProject(gAudacityProjects[i].get()))
         {
            //this assumes tasks are only associated with one project.
            gAudacityProjects[i]->GetEventHandler()->AddPendingEvent(event);
            //mark the changes so that the project can be resaved.
            gAudacityProjects[i]->GetUndoManager()->SetODChangesFlag();
            break;
         }
      }
      --**/

//      wxPrintf("%s %i complete\n", GetTaskName(),GetTaskNumber());
   }
   mTerminateMutex.unlock();
   SetIsRunning(false);
   mBlockUntilTerminateMutex.unlock();
}

/**--
bool ODTask::IsTaskAssociatedWithProject(AudacityProject* proj)
{
   TrackList *tracks = proj->GetTracks();
   TrackListIterator iter1(tracks);
   Track *tr = iter1.First();

   while (tr)
   {
      //go over all tracks in the project
      if (tr->GetKind() == Track::Wave)
      {
         //look inside our task's track list for one that matches this projects one.
         mWaveTrackMutex.lock();
         for(int i=0;i<(int)mWaveTracks.size();i++)
         {
            if(mWaveTracks[i]==tr)
            {
               //if we find one, then the project is associated with us;return true
               mWaveTrackMutex.unlock();
               return true;
            }
         }
         mWaveTrackMutex.unlock();
      }
      tr = iter1.Next();
   }
   return false;

}
--**/

void ODTask::ODUpdate()
{
   Update();
   ResetNeedsODUpdate();
}

void ODTask::SetIsRunning(bool value)
{
   mIsRunningMutex.lock();
   mIsRunning=value;
   mIsRunningMutex.unlock();
}

bool ODTask::IsRunning()
{
   bool ret;
   mIsRunningMutex.lock();
   ret= mIsRunning;
   mIsRunningMutex.unlock();
   return ret;
}

sampleCount ODTask::GetDemandSample() const
{
   sampleCount retval;
   mDemandSampleMutex.lock();
   retval = mDemandSample;
   mDemandSampleMutex.unlock();
   return retval;
}

void ODTask::SetDemandSample(sampleCount sample)
{

   mDemandSampleMutex.lock();
   mDemandSample=sample;
   mDemandSampleMutex.unlock();
}


///return the amount of the task that has been completed.  0.0 to 1.0
float ODTask::PercentComplete()
{
   mPercentCompleteMutex.lock();
   float ret = mPercentComplete;
   mPercentCompleteMutex.unlock();
   return ret;
}

///return
bool ODTask::IsComplete()
{
   return PercentComplete() >= 1.0 && !IsRunning();
}


WaveTrack* ODTask::GetWaveTrack(int i)
{
   WaveTrack* track = NULL;
   mWaveTrackMutex.lock();
   if(i<(int)mWaveTracks.size())
      track = mWaveTracks[i];
   mWaveTrackMutex.unlock();
   return track;
}

///Sets the wavetrack that will be analyzed for ODPCMAliasBlockFiles that will
///have their summaries computed and written to disk.
void ODTask::AddWaveTrack(WaveTrack* track)
{
   mWaveTracks.push_back(track);
}


int ODTask::GetNumWaveTracks()
{
   int num;
   mWaveTrackMutex.lock();
   num = (int)mWaveTracks.size();
   mWaveTrackMutex.unlock();
   return num;
}


void ODTask::SetNeedsODUpdate()
{
   mNeedsODUpdateMutex.lock();
   mNeedsODUpdate=true;
   mNeedsODUpdateMutex.unlock();
}
bool ODTask::GetNeedsODUpdate()
{
   bool ret;
   mNeedsODUpdateMutex.lock();
   ret=mNeedsODUpdate;
   mNeedsODUpdateMutex.unlock();
   return ret;
}

void ODTask::ResetNeedsODUpdate()
{
   mNeedsODUpdateMutex.lock();
   mNeedsODUpdate=false;
   mNeedsODUpdateMutex.unlock();
}

///does an od update and then recalculates the data.
void ODTask::RecalculatePercentComplete()
{
   if(GetNeedsODUpdate())
   {
      ODUpdate();
      CalculatePercentComplete();
   }
}

///changes the tasks associated with this Waveform to process the task from a different point in the track
///@param track the track to update
///@param seconds the point in the track from which the tasks associated with track should begin processing from.
void ODTask::DemandTrackUpdate(WaveTrack* track, double seconds)
{
   bool demandSampleChanged=false;
   mWaveTrackMutex.lock();
   for(size_t i=0;i<mWaveTracks.size();i++)
   {
      if(track == mWaveTracks[i])
      {
         auto newDemandSample = (sampleCount)(seconds * track->GetRate());
         demandSampleChanged = newDemandSample != GetDemandSample();
         SetDemandSample(newDemandSample);
         break;
      }
   }
   mWaveTrackMutex.unlock();

   if(demandSampleChanged)
      SetNeedsODUpdate();

}


void ODTask::StopUsingWaveTrack(WaveTrack* track)
{
   mWaveTrackMutex.lock();
   for(size_t i=0;i<mWaveTracks.size();i++)
   {
      if(mWaveTracks[i] == track)
         mWaveTracks[i]=NULL;
   }
   mWaveTrackMutex.unlock();
}

///Replaces all instances to a wavetrack with a NEW one, effectively transferring the task.
void ODTask::ReplaceWaveTrack(WaveTrack* oldTrack,WaveTrack* newTrack)
{
   mWaveTrackMutex.lock();
   for(size_t i=0;i<mWaveTracks.size();i++)
   {
      if(oldTrack == mWaveTracks[i])
      {
         mWaveTracks[i] = newTrack;
      }
   }
   mWaveTrackMutex.unlock();
}
