#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <map>
#include <limits.h>


#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define PARENT_PID (-1)
#define COMMAND_MAX_LENGTH (80)

class JobsList;

class Command {
protected:
    char** args;
    int num_args;
public:
    JobsList* jobs_list_ptr;
    pid_t pid;
    time_t called_time;
    char* cmd_line;
	bool is_background;
	bool forced_stop;
    Command(const char* _cmd_line, JobsList* job_list_ptr, pid_t _pid = PARENT_PID, bool forced_stop = false);
    virtual ~Command() {}
    virtual void execute() = 0;
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line, JobsList* jobs_list_ptr, pid_t pid = PARENT_PID) : Command(cmd_line, jobs_list_ptr, pid) {}
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
  bool is_complex;
  public:
    ExternalCommand(const char* cmd_line, JobsList* jobs_list_ptr, pid_t _pid);
    virtual ~ExternalCommand() {}
    void execute() override;
    void executeSimple();
    void executeComplex();
};

class PipeCommand : public Command {
public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
};

class ChpromptCommand : public BuiltInCommand {
private:
    std::string *prompt_name;
public:
    ChpromptCommand(const char* cmd_line, JobsList* jobs_list_ptr, std::string *name);
    virtual ~ChpromptCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    char** lastPwd;
    ChangeDirCommand(const char* cmd_line, char** plastPwd, JobsList* jobs_list_ptr) :
            BuiltInCommand(cmd_line, jobs_list_ptr), lastPwd(plastPwd) {}
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line, JobsList* jobs_list_ptr) : BuiltInCommand(cmd_line,jobs_list_ptr) {}
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line, JobsList* jobs_list_ptr) : BuiltInCommand(cmd_line, jobs_list_ptr) {}
    virtual ~ShowPidCommand() {}
    void execute() override;
};


class QuitCommand : public BuiltInCommand {

public:
    QuitCommand(const char* cmd_line, JobsList* jobs_list_ptr)
            : BuiltInCommand(cmd_line, jobs_list_ptr) {}
    virtual ~QuitCommand() {}
    void execute() override;
};

class JobEntry
{
public:
    int job_id;
    Command* cmd;
    bool isStopped;
    time_t start_time;
    JobEntry(int job_id, Command* cmd, bool stopped) :
            job_id(job_id), cmd(cmd), isStopped(stopped) {}
};

class JobsList
{
public:
    Command* command_in_fg=nullptr;
    std::map<int, JobEntry*> jobs_list;
    JobsList() = default;
    ~JobsList() = default;
    void addJob(Command* cmd, bool isStopped = false);
    void printJobById(int jobId);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry* getJobById(int jobId);
    void removeJobById(int jobId);
};

class JobsCommand : public BuiltInCommand {
public:
    JobsCommand(const char* cmd_line, JobsList* jobs_list_ptr)
            : BuiltInCommand(cmd_line, jobs_list_ptr){}
    virtual ~JobsCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs_list_ptr)
            : BuiltInCommand(cmd_line, jobs_list_ptr){}
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs_list_ptr)
            : BuiltInCommand(cmd_line, jobs_list_ptr){}
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
std::string curr_command;
int sleep_time;
public:
    explicit TimeoutCommand(const char* cmd_line, JobsList* jobs_list_ptr, pid_t pid);
    virtual ~TimeoutCommand() {}
    void execute() override;
};

class FareCommand : public BuiltInCommand {
public:
    FareCommand(const char* cmd_line);
    virtual ~FareCommand() {}
    void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
public:
    SetcoreCommand(const char* cmd_line);
    virtual ~SetcoreCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
public:
    KillCommand(const char* cmd_line, JobsList* jobs_list_ptr)
        : BuiltInCommand(cmd_line,jobs_list_ptr){}
    virtual ~KillCommand() {}
    void execute() override;
};

class SmallShell {
private:
    SmallShell();
public:
    class TimeoutEntry{
    public:
        std::string  cmd_name;
        time_t timeOut;
        int job_id;
        pid_t pid;
        TimeoutEntry(std::string cmd_name, time_t timeOut, int job_id, pid_t pid):
        cmd_name(cmd_name), timeOut(timeOut), job_id(job_id), pid(pid){}
    };

    std::string prompt_name;
    char* plastPwd;
    JobsList jobs;
    std::map<pid_t, TimeoutEntry> timed_map;

    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    void setAlarm();
};

#endif //SMASH_COMMAND_H_
