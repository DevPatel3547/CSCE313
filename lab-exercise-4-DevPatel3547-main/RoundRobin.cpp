#include "RoundRobin.h"
#include <algorithm>

// Constructor
RoundRobin::RoundRobin(string file, int time_quantum) : time_quantum(time_quantum) {
    extractProcessInfo(file);
    sort(processVec.begin(), processVec.end(), [](const Process &p1, const Process &p2) {
        return p1.get_arrival_time() < p2.get_arrival_time();
    });
}

// Schedule tasks based on RoundRobin Rule
// the jobs are put in the order the arrived
// Make sure you print out the information like we put in the document
void RoundRobin::schedule_tasks() {
    int system_time = 0;
    vector<Process> readyQueue;
    int current_process_index = 0;
    int time_quantum_left = time_quantum;
    bool isProcessRunning = false;

    while (!processVec.empty() || !readyQueue.empty() || isProcessRunning) {
        // put all the processes that arrived by this system time to the readyQueue
        while (!processVec.empty() && processVec.front().get_arrival_time() <= system_time) {
            readyQueue.push_back(processVec.front());
            processVec.erase(processVec.begin());
        }

        // check if any process is running
        if (isProcessRunning) {
            // check if the current process is completed
            if (readyQueue[current_process_index].is_Completed()) {
                print(system_time, readyQueue[current_process_index].getPid(), true);
                readyQueue.erase(readyQueue.begin() + current_process_index);
                isProcessRunning = false;
            }
            // check if time quantum is over
            else if (time_quantum_left == 0) {
                readyQueue.push_back(readyQueue[current_process_index]);
                readyQueue.erase(readyQueue.begin() + current_process_index);
                isProcessRunning = false;
            }
            else {
                readyQueue[current_process_index].Run(1);
                time_quantum_left--;
            }
        }

        // check if there is any process in the readyQueue
        if (!readyQueue.empty() && !isProcessRunning) {
            isProcessRunning = true;
            current_process_index = 0;
            time_quantum_left = time_quantum;
        }

        // update the system time
        system_time++;

        // print status of current process and ready queue
        if (isProcessRunning) {
            print(system_time, readyQueue[current_process_index].getPid(), false);
        }
        else {
            print(system_time, -1, false);
        }
    }
}


/*************************** 
ALL FUNCTIONS UNDER THIS LINE ARE COMPLETED FOR YOU
You can modify them if you'd like, though :)
***************************/


// Default constructor
RoundRobin::RoundRobin() {
	time_quantum = 0;
}

// Time quantum setter
void RoundRobin::set_time_quantum(int quantum) {
	this->time_quantum = quantum;
}

// Time quantum getter
int RoundRobin::get_time_quantum() {
	return time_quantum;
}

// Print function for outputting system time as part of the schedule tasks function
void RoundRobin::print(int system_time, int pid, bool isComplete){
	string s_pid = pid == -1 ? "NOP" : to_string(pid);
	cout << "System Time [" << system_time << "].........Process[PID=" << s_pid << "] ";
	if (isComplete)
		cout << "finished its job!" << endl;
	else
		cout << "is Running" << endl;
}

// Read a process file to extract process information
// All content goes to proces_info vector
void RoundRobin::extractProcessInfo(string file){
	// open file
	ifstream processFile (file);
	if (!processFile.is_open()) {
		perror("could not open file");
		exit(1);
	}

	// read contents and populate process_info vector
	string curr_line, temp_num;
	int curr_pid, curr_arrival_time, curr_burst_time;
	while (getline(processFile, curr_line)) {
		// use string stream to seperate by comma
		stringstream ss(curr_line);
		getline(ss, temp_num, ',');
		curr_pid = stoi(temp_num);
		getline(ss, temp_num, ',');
		curr_arrival_time = stoi(temp_num);
		getline(ss, temp_num, ',');
		curr_burst_time = stoi(temp_num);
		Process p(curr_pid, curr_arrival_time, curr_burst_time);

		processVec.push_back(p);
	}

	// close file
	processFile.close();
}
