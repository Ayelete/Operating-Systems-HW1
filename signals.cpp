#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) 
{
    //We have to keep a pointer to the command that is running in the foreground
    //update it in the pipes commands

	cout << "smash: got ctrl-Z" <<endl;
    SmallShell& smash= SmallShell::getInstance();

    if(smash.jobs.command_in_fg != nullptr)
    {
        Command *fg_cmd = smash.jobs.command_in_fg;
        if(fg_cmd->forced_stop) {
			fg_cmd->forced_stop=false;
		}
		else
		{
			smash.jobs.addJob(new ExternalCommand(fg_cmd->cmd_line, &smash.jobs, fg_cmd->pid), true);
		}
        smash.jobs.command_in_fg = nullptr;
        if(kill(fg_cmd->pid, SIGSTOP) == -1)
        {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " << fg_cmd->pid << " was stopped" <<endl;
    }
}

void ctrlCHandler(int sig_num) 
{
    cout << "smash: got ctrl-C" <<endl;
    SmallShell& smash= SmallShell::getInstance();

    if(smash.jobs.command_in_fg!= nullptr)
    {
        Command* fg_cmd=smash.jobs.command_in_fg;
        if(kill(fg_cmd->pid, SIGKILL) == -1)
        {
            perror("smash error: kill failed");
            return;
        }
        cout <<"smash: process " << fg_cmd->pid <<" was killed" << endl;


        smash.jobs.removeFinishedJobs();
    }

}

void alarmHandler(int sig_num)
{
    cout << "smash: got an alarm" <<endl;

    SmallShell &smash= SmallShell::getInstance();

    time_t curr_time;
    time(&curr_time);

    for(auto it = smash.timed_map.begin(); it!=smash.timed_map.end();)   //if we have few commands to kill at the same times we have to pass find and to kill all of them
    {

        if(it->second.timeOut<=curr_time)
        {
            if((smash.jobs.command_in_fg!= nullptr && smash.jobs.command_in_fg->pid == it->first) || (it->second.job_id>0))
            {
               if(kill(it->second.pid, SIGKILL)==-1)
               {
                   perror("smash error: kill failed");
                   return;
               }
               smash.jobs.removeFinishedJobs();
               if((it->second.job_id>0) && smash.jobs.jobs_list.find(it->second.job_id) != smash.jobs.jobs_list.end())
               {
				   smash.jobs.jobs_list.erase(it->second.job_id);
				   cout << "smash: " << it->second.cmd_name << " timed out!" << endl;   
			   }
			   else if(it->second.job_id<=0)
			   {
				   cout << "smash: " << it->second.cmd_name << " timed out!" << endl;
			   }
            }
            smash.timed_map.erase(it->second.pid);
            it=smash.timed_map.begin();

        }
        else{
            it++;
        }
    }
    smash.setAlarm();
}
