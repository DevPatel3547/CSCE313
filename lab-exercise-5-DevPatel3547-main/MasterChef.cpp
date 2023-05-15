/*Name: Dev RahulBhai Patel
UIN: 931007975 */

#include <getopt.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include "StepList.h"

using namespace std;

StepList* recipeSteps;
vector<int>* completedSteps;
int completeCount = 0;

void PrintHelp()
{
	cout << "Usage: ./MasterChef -i <file>\n\n";
	cout<<"--------------------------------------------------------------------------\n";
	cout<<"<file>:    "<<"csv file with Step, Dependencies, Time (m), Description\n";
	cout<<"--------------------------------------------------------------------------\n";
	exit(1);
}

string ProcessArgs(int argc, char** argv)
{
	string result = "";
	// print help if odd number of args are provided
	if (argc < 3) {
		PrintHelp();
	}

	while (true)
	{
		const auto opt = getopt(argc, argv, "i:h");

		if (-1 == opt)
			break;

		switch (opt)
		{
		case 'i':
			result = std::string(optarg);
			break;
		case 'h': // -h or --help
		default:
			PrintHelp();
			break;
		}
	}

	return result;
}

void makeTimer( Step *timerID, int expire)
{
    struct sigevent te;
    struct itimerspec its;

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = SIGRTMIN;
    te.sigev_value.sival_ptr = timerID;
    timer_create(CLOCK_REALTIME, &te, &(timerID->t_id));

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = expire;
    its.it_value.tv_nsec = 0;
    timer_settime(timerID->t_id, 0, &its, NULL);
}



void RemoveDepHandler(int sig) {
	/* Section 3 */
	// Foreach step that has been completed since last run, remove it as a dependency
	for (int i = 0; i < completedSteps->size(); i++) {
		recipeSteps->RemoveDependency(completedSteps->at(i));
	}
	completedSteps->clear();
	/* End Section 3 */
}

static void timerHandler( int sig, siginfo_t *si, void *uc )
{
    Step* comp_item = (Step*)si->si_value.sival_ptr;

	/* Section 2 */
	// Officially complete the step using completedSteps and completeCount
	completedSteps->push_back(comp_item->id);
	completeCount++;

	// Ready to remove that dependency, call the trigger for the appropriate handler
	RemoveDepHandler(sig);
	/* End Section 2 */
}


int main(int argc, char **argv)
{
    string input_file = ProcessArgs(argc, argv);
    if (input_file.empty()) {
        exit(1);
    }

    // Initialize global variables
    completedSteps = new vector<int>();
    recipeSteps = new StepList(input_file);

    /* Set up signal handler. */
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timerHandler;
    sigemptyset(&sa.sa_mask);

    /* Associate the signal SIGRTMIN with the sa using the sigaction function */
    if (sigaction(SIGRTMIN, &sa, nullptr) == -1) {
        perror("Error: sigaction");
        exit(EXIT_FAILURE);
    }

    /* Associate the appropriate handler with the SIGUSR1 signal, for removing dependencies */
    signal(SIGUSR1, RemoveDepHandler);

    // Until all steps have been completed, check if steps are ready to be run and create a timer for them if so
    while (completeCount < recipeSteps->Count()) {
        vector<Step*> readySteps = recipeSteps->GetReadySteps();
        for (auto step : readySteps) {
            makeTimer(step, step->duration);
        }
        sleep(1);
    }

    cout << "Enjoy!" << endl;
	// delete recipeSteps;
	// delete completedSteps;

    return 0;
}
