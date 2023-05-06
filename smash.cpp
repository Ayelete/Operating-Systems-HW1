#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    struct sigaction alarm_sa;
    alarm_sa.sa_handler= &alarmHandler;
    alarm_sa.sa_flags=SA_RESTART;

    if(sigaction(SIGALRM, &alarm_sa, NULL)==-1)
    {
        perror("smash error: failed to set alarm handler");
    }

    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        //std::cout << "done1" << std::endl;
        std::cout << smash.prompt_name << "> ";
        //std::cerr << "check123" << std::endl;
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());

    }

    return 0;
}
