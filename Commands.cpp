#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \


  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}


bool _isBackgroundCommand(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

bool _isComplexCommand(basic_string<char> cmd_line) {
    string str(cmd_line);
    return str.find('*') != string::npos || str.find('?') != string::npos;
}

void _removeBackgroundSign(char* cmd_line) {
    if(strlen(cmd_line) <= 1 && *cmd_line!='&')
        return;
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

char** getIOPipeCommand(const char* cmd_line)
{
    char** cmd = new char*[3];
    for(int i=0;i<3;i++) {
        cmd[i] = new char[COMMAND_MAX_LENGTH];
        for(int j=0;j<COMMAND_MAX_LENGTH;j++)
            cmd[i][j]='\0';
    }

    int pos_op=0, size_op=(-1);
    for(; cmd_line[pos_op] ; pos_op++)
    {
        if(cmd_line[pos_op] == '>')
        {
            if(pos_op < COMMAND_MAX_LENGTH-1 && cmd_line[pos_op+1] == '>')
            {
                strcpy(cmd[1], ">>\0");
                size_op=2;
            }
            else
            {
                strcpy(cmd[1], ">\0");
                size_op=1;
            }
            break;
        }
        if(cmd_line[pos_op] == '|')
        {
            if(pos_op < COMMAND_MAX_LENGTH-1 && cmd_line[pos_op+1] == '&')
            {
                strcpy(cmd[1], "|&\0");
                size_op=2;
            }
            else
            {
                strcpy(cmd[1], "|\0");
                size_op=1;
            }
            break;
        }
    }

    if(size_op == -1)
    {
        for(int i=0;i<3;i++)
            delete[] cmd[i];
        delete[] cmd;
        return nullptr;
    }
    strncpy(cmd[0], cmd_line ,pos_op);
    if(pos_op < COMMAND_MAX_LENGTH)
        cmd[0][pos_op] = '\0';
    strcpy(cmd[2], cmd_line+pos_op+size_op);
    string fixed = _trim(cmd[2]); // we need to remove the spaces
    strcpy(cmd[2], fixed.c_str());
    _removeBackgroundSign(cmd[2]);
    return cmd;
}


/////////////// COMMAND /////////////
Command::Command(const char* _cmd_line, JobsList* jobs_list_ptr, pid_t _pid, bool forced_stop): jobs_list_ptr(jobs_list_ptr)
{
    args = new char*[20];
    cmd_line=new char[COMMAND_MAX_LENGTH];
    for(int i=0;i<COMMAND_MAX_LENGTH;i++)
        cmd_line[i]='\0';
    strcpy(cmd_line, _cmd_line);
    _removeBackgroundSign(cmd_line);
    num_args=_parseCommandLine(cmd_line, args);
    if(_pid != PARENT_PID)
    {
        pid=_pid;
    }
    else
    {
        pid=getpid();
    }
    time(&called_time);
    strcpy(cmd_line,_cmd_line);
    is_background=_isBackgroundCommand(_cmd_line);
}

/////////////// CH PROMPT //////////////
ChpromptCommand::ChpromptCommand(const char* cmd_line, JobsList* jobs_list_ptr, std::string* name)
        : BuiltInCommand(cmd_line, jobs_list_ptr), prompt_name(name) {}

void ChpromptCommand::execute()
{
    if(num_args == 1)
    {
        *prompt_name=std::string("smash");
    }
    else
    {
        *prompt_name=std::string(args[1]);
    }
}

////////////// SHOW PID //////////////
void ShowPidCommand::execute()
{
    std::cout << "smash pid is " << getpid() << endl;
}

//////// CHANGE DIR //////////////
void ChangeDirCommand::execute()
{
	if(num_args < 2)
	{
		std::cerr << "smash error:> " << '"' << cmd_line << '"' << endl;
		return;
	}
    if(num_args > 2) {
        std::cerr << "smash error: cd: too many arguments" << endl;
        return;

    }
    else if(!strcmp(args[1],"-") && !strcmp(*lastPwd,"")){
        std::cerr << "smash error: cd: OLDPWD not set" << endl;
    }
    else
    {
        char* cur_path = (char*)malloc(PATH_MAX * sizeof(char));
        getcwd(cur_path,PATH_MAX);
        if(!strcmp(args[1],"-"))
        {
            if(chdir(*lastPwd) != 0) {
                perror("smash error: chdir failed");
                return;
            }
        }
        else
        {
            if(chdir(args[1]) != 0) {
                perror("smash error: chdir failed");
                return;
            }
        }
        strcpy(*lastPwd, cur_path);
        free(cur_path);
    }
}

////////////// GET CUR DIR /////////////
void GetCurrDirCommand::execute()
{
    char* path = (char*)malloc(PATH_MAX*sizeof(char));
    if(getcwd(path, PATH_MAX))
    {
        cout<<path<<endl;
    }
    free(path);
}

/////////////// JOBS //////////////////////

void JobsList::addJob(Command* cmd, bool isStopped)
{
    if(!this->jobs_list.empty())
        this->removeFinishedJobs();
    if(jobs_list.empty())
    {
        JobEntry* new_job = new JobEntry(1, cmd, isStopped);
        this->jobs_list.insert(std::pair<int,JobEntry*>(1, new_job));
    }
    else
    {
        int max=(jobs_list.rbegin())->first;
        JobEntry* new_job = new JobEntry(max+1, cmd, isStopped);
        this->jobs_list.insert(std::pair<int,JobEntry*>(max+1, new_job));
    }
}
void JobsList::printJobById(int jobId)
{
    JobEntry* job = getJobById(jobId);
    Command* cmd = (job->cmd);
    time_t cur_time;
    time(&cur_time);
    cout << "[" << job->job_id << "]" << " ";
    cout << cmd->cmd_line << " : " << cmd->pid << " ";
    cout << difftime(cur_time, cmd->called_time) << " secs";
    if(job->isStopped)
        cout << " (stopped)";
    cout << endl;
}
void JobsList::printJobsList()
{
    for (auto itr = jobs_list.begin() ; itr!=jobs_list.end(); ++itr)
    {
        int job_id = itr->first;
        printJobById(job_id);
    }
}
void JobsList::killAllJobs()
{
    cout << "smash: sending SIGKILL signal to " << jobs_list.size() << " jobs:" << endl;
    for(auto itr = jobs_list.begin() ; itr!=jobs_list.end(); ++itr)
    {
        JobEntry* job = itr->second;
        Command *cmd = (job->cmd);
        pid_t pid = cmd->pid;
        cout << pid << ": " << cmd->cmd_line << endl;
        if(kill(pid,9) == -1)
        {
            perror("smash error: kill failed");
            return;
        }
    }
}
void JobsList::removeFinishedJobs()
{
    auto itr = jobs_list.begin();
    while(itr!=jobs_list.end() && !jobs_list.empty())
    {
        JobEntry* job = itr->second;
        if(waitpid((job->cmd)->pid,nullptr,WNOHANG) == (job->cmd)->pid)
        {
            jobs_list.erase(itr->first);
            itr=jobs_list.begin();
        }
        else
            itr++;
    }
}

JobEntry* JobsList::getJobById(int jobId)
{
    return (jobs_list.find(jobId)->second);
}

void JobsList::removeJobById(int jobId)
{
    jobs_list.erase(jobId);
}

void JobsCommand::execute()
{
    jobs_list_ptr->removeFinishedJobs();
    jobs_list_ptr->printJobsList();
}

/////////// FG COMMAND /////////////////
void ForegroundCommand::execute()
{
    int job_id;
    JobEntry* job;
    if(num_args != 1 && num_args !=2)
    {
        std::cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    else if(num_args == 2)
    {
        char* id = args[1];
        unsigned int i=0;
        while(i<strlen(id))
        {
            if((*id<'0' || *id>'9') && *id != '-')
            {
                std::cerr << "smash error: fg: invalid arguments" << endl;
                return;
            }
            i++;
        }
        job_id=std::stoi(std::string(args[1]), nullptr, 10);
        if(jobs_list_ptr->jobs_list.find(job_id) == jobs_list_ptr->jobs_list.end())
        {
            std::cerr << "smash error: fg: job-id " << job_id << " does not exist" << endl;
            return;
        }
        job = (jobs_list_ptr->jobs_list.find(job_id)->second);
    }
    else
    {
        if(jobs_list_ptr->jobs_list.empty())
        {
            std::cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }
        job = (jobs_list_ptr->jobs_list.rbegin())->second;
        job_id = (jobs_list_ptr->jobs_list.rbegin())->first;
    }
    Command* cmd = (job->cmd);
    pid_t pid = cmd->pid;
    cout << cmd->cmd_line << " : " << pid << endl;
    if(kill(pid,18) == -1)
    {
        perror("smash error: kill failed");
        return;
    }
    cmd->forced_stop=true;
    jobs_list_ptr->command_in_fg = cmd;
    waitpid(pid,nullptr, WUNTRACED);
    jobs_list_ptr->command_in_fg = nullptr;
    if(!cmd->forced_stop) {
        job->isStopped=true;
        forced_stop=false;
    }
    else
    {
        jobs_list_ptr->removeJobById(job_id);
    }
}

///////// bg command //////////////
void BackgroundCommand::execute()
{
    int job_id = -1;
    JobEntry* job;
    if(num_args != 1 && num_args !=2)
    {
        std::cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }
    else if(num_args == 2)
    {
        char* id = args[1];
        while(id!=nullptr && *id!='\0')
        {
            if((*id<'0' || *id>'9') && *id != '-')
            {
                std::cerr << "smash error: bg: invalid arguments" << endl;
                return;
            }
            id++;
        }
        job_id=std::stoi(std::string(args[1]), nullptr, 10);
        if(jobs_list_ptr->jobs_list.find(job_id) == jobs_list_ptr->jobs_list.end())
        {
            std::cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
            return;
        }
        job = (jobs_list_ptr->jobs_list.find(job_id)->second);
        if(!job->isStopped)
        {
            std::cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
            return;
        }
    }
    else
    {
        for(map<int,JobEntry*>::reverse_iterator itr = jobs_list_ptr->jobs_list.rbegin() ; itr!=jobs_list_ptr->jobs_list.rend(); ++itr)
        {
            if((itr->second)->isStopped) {
                job_id=itr->first;
                job=(itr->second);
                break;
            }
        }
        if(job_id==-1) {
            std::cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
    }
    Command* cmd = (job->cmd);
    pid_t pid = cmd->pid;
    cout << cmd->cmd_line << " : " << pid << endl;
    if(kill(pid,18) == -1)
    {
        perror("smash error: kill failed");
        return;
    }
    job->isStopped=false;
}

//////////// QUIT COMMAND //////////////
void QuitCommand::execute()
{
    if(num_args > 1 && !strcmp(args[1],"kill"))
    {
        jobs_list_ptr->killAllJobs();
        jobs_list_ptr->removeFinishedJobs();
    }
    exit(0);
}

////////////// KILL COMMAND /////////////
void KillCommand::execute()
{
    if(num_args !=3)
    {
        std::cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    char *signum=args[1], *id=args[2];
    if(signum[0]!='-')
    {
        std::cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    signum++;
    while(signum!=nullptr && *signum!='\0')
    {
        if(*signum<'0' || *signum>'9')
        {
            std::cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        signum++;
    }
    while(id!=nullptr && *id!='\0')
    {
        if((*id<'0' || *id>'9') && *id != '-')
        {
            std::cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        id++;
    }
    signum = args[1];
    signum++;
    int job_id=std::stoi(std::string(args[2]), nullptr, 10);
    int sig=std::stoi(std::string(signum), nullptr, 10);
    if(sig > 64 || sig < 1)
    {
        std::cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    if(jobs_list_ptr->jobs_list.find(job_id) == jobs_list_ptr->jobs_list.end())
    {
        std::cerr << "smash error: kill: job-id " << job_id << " does not exist" << endl;
        return;
    }
    JobEntry* job = (jobs_list_ptr->jobs_list.find(job_id)->second);
    if(kill((job->cmd)->pid, sig) == -1)
    {
        perror("smash error: kill failed");
        return;
    }
    jobs_list_ptr->removeFinishedJobs();
    std::cout << "signal number " << sig << " was sent to pid " << (job->cmd)->pid << endl;
}

/////////////// EXTERNAL COMMAND /////////////////////

ExternalCommand::ExternalCommand(const char* cmd_line, JobsList* job_list_ptr, pid_t _pid) : Command(cmd_line, job_list_ptr, _pid){
    is_complex=_isComplexCommand(cmd_line);
}

void ExternalCommand::execute()
{
    if(is_complex)
        executeComplex();
    else
        executeSimple();

}

void ExternalCommand::executeSimple()
{
    pid_t pid = fork();
    if(pid==0)
    {
        setpgrp();
        if(execvp(args[0], args) == -1)
        {
            perror("smash error: execv failed");
            exit(1);
        }
    }
    else if(pid>0)
    {
        this->pid=pid;
        if(is_background)
        {
            jobs_list_ptr->addJob(new ExternalCommand(cmd_line, jobs_list_ptr, pid), false);
        }
        else
        {
            jobs_list_ptr->command_in_fg=this;
            waitpid(pid,nullptr, WUNTRACED);
            jobs_list_ptr->command_in_fg=nullptr;
        }
    }
    else
    {
        perror("smash error: fork failed");
    }
}

void ExternalCommand::executeComplex()
{
    pid_t pid = fork();
    if(pid==0)
    {
        setpgrp();
        char* _cmd_line=(char*)malloc(sizeof(char)*COMMAND_MAX_LENGTH);
        for(int i=0;i<COMMAND_MAX_LENGTH;i++)
            _cmd_line[i]='\0';
        strcpy(_cmd_line, cmd_line);
        _removeBackgroundSign(_cmd_line);
        char* bash_args[] = {(char*)"bash", (char*)"-c", _cmd_line ,nullptr};
        if(execv("/bin/bash", bash_args)== -1)
        {
            perror("smash error: execv failed");
            exit(1);
        }
    }
    else if(pid>0)
    {
        this->pid=pid;
        if(is_background)
        {
            jobs_list_ptr->addJob(new ExternalCommand(cmd_line, jobs_list_ptr, pid), false);
        }
        else
        {
            jobs_list_ptr->command_in_fg=this;
            waitpid(pid,nullptr, WUNTRACED);
            jobs_list_ptr->command_in_fg=nullptr;
        }
    }
    else
    {
        perror("smash error: fork failed");
    }
}

////////////////// TIME OUT COMMAND /////////////////////
TimeoutCommand::TimeoutCommand(const char *cmd_line, JobsList *jobs_list_ptr, pid_t pid) : BuiltInCommand(cmd_line, jobs_list_ptr, pid)
{
     if(num_args>1)
         sleep_time= atoi(args[1]);
    curr_command="";
    for(int i=2; i<num_args; i++)
    {
        curr_command+=std::string(args[i])+ " ";
    }
}

void TimeoutCommand::execute()
{
    if(num_args<3)
    {
        //std::cerr << "smash error: timeout: invalid arguments" << endl;
		std::cerr << "smash error:> " << '"' << cmd_line << '"' << endl;
        return;
    }
    char* duration = args[1];
    unsigned int i=0;
    while(i<strlen(duration))
    {
        if((*duration<'0' || *duration>'9') )
        {
            //std::cerr << "smash error: timeout: invalid arguments" << endl;
			std::cerr << "smash error:> " << '"' << cmd_line << '"' << endl;
            return;
        }
        i++;
    }
    if(num_args==3 && *args[1]<0)
    {
        //std::cerr << "smash error: timeout: invalid arguments" << endl;
		std::cerr << "smash error:> " << '"' << cmd_line << '"' << endl;
        return;
    }
    pid_t pid =fork();

    if(pid==0) //child
    {
        setpgrp();
        std::string cleanCmd= curr_command;
        _removeBackgroundSign((char*)cleanCmd.c_str());
        char** cmd_args = new char*[18];
        for(int i=0;i<18;i++)
        {
			cmd_args[i]=args[i+2];
		}
        if(_isComplexCommand(cleanCmd))
        {
            char* bash_args[] = {(char*)"bash", (char*)"-c", (char*)cleanCmd.c_str(), nullptr};
            if(execv("/bin/bash", bash_args)== -1)
            {
                perror("smash error: execv failed");
                exit(1);
            }
        }
        else{
            if(execvp(cmd_args[0], cmd_args) == -1)
            {
                perror("smash error: execv failed");
                exit(1);
            }
        }
    }
    else if(pid>0)
    {

        time_t timeout, curr_time=0;
        time(&curr_time);
        timeout=curr_time+sleep_time;
        int job_id=-1;
        this->pid=pid;
        if(is_background) {
            jobs_list_ptr->addJob(new TimeoutCommand(cmd_line, jobs_list_ptr, pid), false);
            job_id=(jobs_list_ptr->jobs_list.empty())? 1: jobs_list_ptr->jobs_list.rbegin()->first;
        }

        SmallShell::TimeoutEntry entry= SmallShell::TimeoutEntry(std::string(cmd_line),timeout,job_id,pid);
        SmallShell &smash= SmallShell::getInstance();
        smash.timed_map.insert(pair<pid_t, SmallShell::TimeoutEntry>(pid, entry));

        smash.setAlarm();



        if(!is_background){
            jobs_list_ptr->command_in_fg=this;
            waitpid(pid,nullptr, WUNTRACED);
            jobs_list_ptr->command_in_fg=nullptr;
        }

    }
    else{
        perror("fork failed");
        return;
    }

}




/////////////////// SMALL SHELL ////////////////////////
SmallShell::SmallShell()
{
    prompt_name="smash";
    plastPwd=(char*)malloc(sizeof(char)*PATH_MAX);
    strcpy(plastPwd, "");

}

SmallShell::~SmallShell() {
    free(plastPwd);

}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command* SmallShell::CreateCommand(const char* cmd_line) {
    char* _cmd_line=(char*)malloc(sizeof(char)*COMMAND_MAX_LENGTH);
    for(int i=0;i<COMMAND_MAX_LENGTH;i++)
        _cmd_line[i]='\0';
    strcpy(_cmd_line, cmd_line);
    _removeBackgroundSign(_cmd_line);
    string cmd_s = _trim(string(_cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    free(_cmd_line);
    if(firstWord.compare("chprompt") == 0)
    {
        return new ChpromptCommand(cmd_line, &jobs, &prompt_name);
    }
    else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line, &jobs);
    }
    else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line, &jobs);
    }
    else if(firstWord.compare("cd") == 0)
    {
        return new ChangeDirCommand(cmd_line, &plastPwd, &jobs);
    }
    else if(firstWord.compare("jobs") == 0)
    {
        return new JobsCommand(cmd_line, &jobs);
    }
    else if(firstWord.compare("fg") == 0)
    {
        return new ForegroundCommand(cmd_line, &jobs);
    }
    else if(firstWord.compare("bg") == 0)
    {
        return new BackgroundCommand(cmd_line, &jobs);
    }
    else if(firstWord.compare("quit") == 0)
    {
        return new QuitCommand(cmd_line, &jobs);
    }
    else if(firstWord.compare("kill") == 0)
    {
        return new KillCommand(cmd_line, &jobs);
    }
    else if(firstWord.compare("timeout")==0)
    {
        return  new TimeoutCommand(cmd_line, &jobs, PARENT_PID);
    }
    else {
        return new ExternalCommand(cmd_line, &jobs, PARENT_PID);

    }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line)
{
    char** special_cmd = getIOPipeCommand(cmd_line);
    int new_fd = 0, cur_fd=0, op=-1;
    int new_pipe[2];
    pid_t pid=-1;
    Command* cmd;
    if(special_cmd==nullptr)
    {
        cmd = CreateCommand(cmd_line);
    }
    else // io or pipe command
    {
        // default - op = 0 which is '>'
        if(!strcmp(special_cmd[1], ">>"))
            op=1;
        else if(!strcmp(special_cmd[1], "|"))
            op=2;
        else if(!strcmp(special_cmd[1], "|&"))
            op=3;
        else
            op=0;

        if(op == 0 || op == 1) // IO CMD
        {
            cmd = CreateCommand(special_cmd[0]);
            cur_fd=dup(1);
            // cur_fd refers now to stdout
            if(cur_fd == (-1))
            {
                perror("smash error: dup failed");
                return;
            }
            // we will close 1 so it doesnt refer to stdout
            if(close(1) == (-1))
            {
                perror("smash error: close failed");
                return;
            }
            if(op == 0) // read and override
            {
                new_fd = open(special_cmd[2], O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
            }
            else
            {
                new_fd = open(special_cmd[2], O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
            }
            // new_fd should get 1, since it's the lowest one available
            if(new_fd == (-1))
            {
                // open failed, so we will make 1 refer to stdout
                dup2(cur_fd, 1);
                perror("smash error: open failed");
                return;
            }
        }
        else // PIPE CMD (OP == 2 OR OP == 3)
        {
            if(pipe(new_pipe) == -1)
            {
                perror("smash error: pipe failed");
            }
            pid = fork();
            if(pid < 0)
                perror("smash error: fork failed");
            else if(pid == 0) // son
            {
                // redirect command2 stdin to pipe's read channel (0)
                if(close(new_pipe[1]) == (-1))
                {
                    perror("smash error: close failed");
                    return;
                }
                dup2(new_pipe[0], 0); // 0 will now refer to new_pipe[0]
                cmd = CreateCommand(special_cmd[2]); // create command 2
            }
            else // parent
            {
                if(close(new_pipe[0]) == (-1))
                {
                    perror("smash error: close failed");
                    return;
                }
                if(op == 2) // redirect command 1 stdout to its write channel
                {
                    cur_fd=dup(1);
                    dup2(new_pipe[1], 1);
                }
                else // redirect command 1 stderr to its write channel
                {
                    cur_fd=dup(2);
                    dup2(new_pipe[1], 2);
                }
                cmd = CreateCommand(special_cmd[0]); // create command 1
            }
        }

        for(int i=0;i<3;i++)
            delete[] special_cmd[i];
        delete[] special_cmd;

    }

    jobs.removeFinishedJobs();
    cmd->execute();
    delete(cmd);

    if(new_fd)
    {
        close(new_fd);
        dup2(cur_fd, 1);
    }
    if(op == 2 || op ==3)
    {
        if(pid > 0) // parent
        {
            if(op == 2)
                dup2(cur_fd, 1);
            else if(op==3)
                dup2(cur_fd, 2);
            close(new_pipe[1]);
            waitpid(pid,nullptr, WUNTRACED | WCONTINUED); // wait for the son to finish
        }
        else if(pid == 0) // exit from the son
        {
            exit(1);
        }
    }
}

void SmallShell::setAlarm()  //we want to find the closest time to put it the alarm
{
    time_t curr_time=0;
    time_t min_time=0;
    time(&curr_time);

    for(auto itr= timed_map.begin(); itr!=timed_map.end();)
    {
        if(itr->second.timeOut<min_time || min_time==0)
        {
            min_time=itr->second.timeOut;
        }
        itr++;
    }

    time_t alarm_time=min_time-curr_time;
    alarm(alarm_time);
}

