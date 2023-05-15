#include <fstream>
#include <iostream>
#include <thread>
#include <sys/time.h>
#include <sys/wait.h>  
#include "BoundedBuffer.h"  
#include "common.h"
#include "Histogram.h"
#include "HistogramCollection.h"
#include "FIFORequestChannel.h"

#include <functional>
#include <cmath>
#include <string>
#include <cstdio>
#include <utility>    
using namespace std;

// ecgno to use for datamsgs
#define EGCNO 1





// This function is executed by the patient threads
void patient_thread_function(int patient_num, int num_requests, int buffer_capacity, int ecg_type, BoundedBuffer& request_buffer) {
    // Set the time step to 0.004 seconds
    double time_step = 0.004;
    // Start the current time at 0.0
    double current_time = 0.0;

    // Loop for the specified number of requests
    for (int i = 0; i < num_requests; i++) {
        // Allocate memory for the buffer
        char* buffer = new char[buffer_capacity];
        // Create a datamsg object with the patient number, current time, and ECG type
        datamsg data(patient_num, current_time, ecg_type);
        // Copy the datamsg object into the buffer
        memcpy(buffer, &data, sizeof(datamsg));
        // Push the buffer onto the request buffer
        request_buffer.push(buffer, sizeof(datamsg));
        // Increment the current time by the time step
        current_time += time_step;
        // Deallocate the buffer memory to avoid memory leaks
        delete[] buffer;
    }
}



// This function is executed by the file thread
void file_thread_function(string filename, int buffer_capacity, int file_size, BoundedBuffer& request_buffer) {
    // Calculate the length of the file message
    int message_length = static_cast<int>(sizeof(filemsg) + (filename.size() + 1));

    // Calculate the number of messages needed to send the entire file
    __int64_t num_messages = file_size / buffer_capacity;
    // Keep track of the accumulated offset
    __int64_t acc = 0;

    // Helper function to handle message creation and buffer pushing
    auto handle_message = [&](__int64_t offset, int length) {
        // Create a filemsg object with the given offset and length
        filemsg file_message(offset, length);
        // Allocate memory for the buffer
        char* buffer = new char[message_length];
        // Copy the file message and filename into the buffer
        memcpy(buffer, &file_message, sizeof(filemsg));
        strcpy(buffer + sizeof(filemsg), filename.c_str());
        // Push the buffer onto the request buffer
        request_buffer.push(buffer, message_length);
        // Deallocate the buffer memory to avoid memory leaks
        delete[] buffer;
    };

    // Loop through each message except the last one
    for (__int64_t i = 0; i < num_messages; ++i) {
        handle_message(acc, buffer_capacity); // Use helper function to handle message
        acc += buffer_capacity; // Update the accumulated offset
    }

    // Handle the last chunk of the file
    __int64_t remaining_offset = file_size % buffer_capacity;
    if (remaining_offset != 0) {
        handle_message(acc, remaining_offset); // Use helper function to handle the last message
    }
}


// Define a custom struct to hold pairs of patient numbers and values
struct PairStruct {
    int p_no;
    double value;
    bool kill = false;
};

// This function is executed by the worker threads
void worker_thread_function(FIFORequestChannel* channel, int buffer_capacity, BoundedBuffer& request_buffer, BoundedBuffer& response_buffer) {
    for (;;) {
        char* buffer = new char[buffer_capacity];
        request_buffer.pop(buffer, buffer_capacity);

        MESSAGE_TYPE message_type = *reinterpret_cast<MESSAGE_TYPE*>(buffer);

        if (message_type == DATA_MSG) {
            // Handle a data message
            datamsg* data_message = reinterpret_cast<datamsg*>(buffer);
            channel->cwrite(buffer, sizeof(datamsg));
            double response;
            channel->cread(&response, sizeof(double));

            // Create a PairStruct to hold the response data
            PairStruct response_data{data_message->person, response, false};

            memcpy(buffer, &response_data, sizeof(PairStruct));
            response_buffer.push(buffer, buffer_capacity);
        } else if (message_type == FILE_MSG) {
            // Handle a file message
            filemsg file_message = *reinterpret_cast<filemsg*>(buffer);
            string filename = buffer + sizeof(filemsg);
            filename = "received/" + filename;
            FILE* file = fopen(filename.c_str(), "r+");

            // Allocate memory for the buffer
            char* buffer_loc = new char[buffer_capacity];

            // Send the file message to the server
            channel->cwrite(buffer, sizeof(filemsg) + filename.size() + 1);
            channel->cread(buffer_loc, buffer_capacity);

            // Write the buffer to the file
            fseek(file, file_message.offset, SEEK_SET);
            fwrite(buffer_loc, 1, file_message.length, file);

            // Close the file and deallocate the buffer memory to avoid memory leaks
            fclose(file);
            delete[] buffer_loc;
        } else if (message_type == QUIT_MSG) {
            // Handle a quit message
            delete[] buffer;
            break;
        }

        // Deallocate the buffer memory to avoid memory leaks
        delete[] buffer;
    }
}


// This function is executed by the histogram threads
void histogram_thread_function(int buffer_capacity, HistogramCollection& collection, BoundedBuffer& response_buffer) {
    for (;;) {
        // Pop a response from the response buffer
        char* buffer = new char[buffer_capacity];
        response_buffer.pop(buffer, buffer_capacity);

        // Extract the PairStruct from the buffer
        PairStruct response_data;
        memcpy(&response_data, buffer, sizeof(PairStruct));

        if (response_data.kill) {
            // Handle a kill message
            delete[] buffer;
            break;
        } else {
            // Update the histogram collection with the response data
            collection.update(response_data.p_no, response_data.value);
        }

        // Deallocate the buffer memory to avoid memory leaks
        delete[] buffer;
    }
}

int main (int argc, char* argv[]) {
    bool process_file = false;
    bool process_patients = false;
    bool process_histogram = false;


    int n = 1000;	// default number of requests per "patient"
    int p = 10;		// number of patients [1,15]
    int w = 100;	// default number of worker threads
	int h = 20;		// default number of histogram threads
    int b = 20;		// default capacity of the request buffer (should be changed)
	int m = MAX_MESSAGE;	// default capacity of the message buffer
	string f = "";	// name of file to be transferred
    //vector<string> channel_names;
    vector<FIFORequestChannel*> channels;
    // read arguments
    int opt;
	while ((opt = getopt(argc, argv, "n:p:w:h:b:m:f:")) != -1) {
		switch (opt) {
			case 'n':
				n = atoi(optarg);
                break;
			case 'p':
				p = atoi(optarg);
                process_patients = true;
                break;
			case 'w':
				w = atoi(optarg);
                break;
			case 'h':
				h = atoi(optarg);
                process_histogram = true; 
				break;
			case 'b':
				b = atoi(optarg);
                break;
			case 'm':
				m = atoi(optarg);
                break;
			case 'f':
				f = optarg;
                process_file = true;
                break;
		}
	}
    
	// fork and exec the server
    int pid = fork();
    if (pid == 0) {
        execl("./server", "./server", "-m", (char*) to_string(m).c_str(), nullptr);
    }
    
	// initialize overhead (including the control channel)
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
    BoundedBuffer response_buffer(b);
	HistogramCollection hc;

    // making histograms and adding to collection
    for (int i = 0; i < p; i++) {
        Histogram* h = new Histogram(10, -2.0, 2.0);
        hc.add(h);
    }
	
	// record start time
    struct timeval start, end;
    gettimeofday(&start, 0);

  // Create file thread
    vector<FIFORequestChannel*> worker_channels;
    for (int i = 0; i < w; i++) {
        MESSAGE_TYPE new_channel_request = NEWCHANNEL_MSG;
        chan->cwrite(&new_channel_request, sizeof(MESSAGE_TYPE));

        char* channel_buffer = new char[m];
        chan->cread(channel_buffer, sizeof(string));
        string channel_name = channel_buffer;

        FIFORequestChannel* worker_channel = new FIFORequestChannel(channel_name, FIFORequestChannel::CLIENT_SIDE);
        worker_channels.push_back(worker_channel);

        delete[] channel_buffer;
    }

    thread* patient_threads = new thread[p];
    thread* worker_threads = new thread[w];
    thread* histogram_threads = new thread[h];
    thread* file_threads = new thread[1];

  
    if (process_file) {
        FILE* file = fopen(("received/" + f).c_str(), "w+");

        filemsg file_message(0, 0);
        string filename = f;
        int message_length = sizeof(filemsg) + (filename.size() + 1);
        char* buffer = new char[message_length];
        memcpy(buffer, &file_message, sizeof(filemsg));
        strcpy(buffer + sizeof(filemsg), filename.c_str());

        chan->cwrite(buffer, message_length);

        __int64_t file_size;
        chan->cread(&file_size, sizeof(__int64_t));

        file_threads[0] = thread(file_thread_function, f, m, file_size, ref(request_buffer));

        delete[] buffer;
        fclose(file);
    }

    // Create worker threads
    for (int i = 0; i < w; i++) {
        FIFORequestChannel* in_channel = worker_channels[i];
        worker_threads[i] = thread(worker_thread_function, in_channel, m, ref(request_buffer), ref(response_buffer));
    }

    // Create patient threads
    if (process_patients) {
        for (int i = 0; i < p; i++) {
            patient_threads[i] = thread(patient_thread_function, (i+1), n, m, 1, ref(request_buffer));
        }
    }

    // Create histogram threads
    if (process_histogram) {
        for (int i = 0; i < h; i++) {
            histogram_threads[i] = thread(histogram_thread_function, m, ref(hc), ref(response_buffer));
        }
    }

    // Join all threads
    if (process_patients) {
        for (int i = 0; i < p; i++) {
            patient_threads[i].join();
        }
    }

    if (process_file) {
        file_threads[0].join();
    }

    for (int i = 0; i < w; i++) {
        MESSAGE_TYPE quit_message = QUIT_MSG;
        request_buffer.push((char*)&quit_message, sizeof(MESSAGE_TYPE));
    }

    for (int i = 0; i < w; i++) {
        worker_threads[i].join();
    }

    for (int i = 0; i < h; i++) {
        PairStruct kill_message;
        kill_message.p_no = 0;
        kill_message.value = 0;
        kill_message.kill = true;
        response_buffer.push((char*)&kill_message, sizeof(PairStruct));
    }

    if (process_histogram) {
        for (int i = 0; i < h; i++) {
            histogram_threads[i].join();
        }
    }

    delete[] patient_threads;
    delete[] worker_threads;
    delete[] histogram_threads;
    delete[] file_threads;



	// record end time
    gettimeofday(&end, 0);

    // print the results
	if (f == "") {
		hc.print();
	}
    int secs = ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) / ((int) 1e6);
    int usecs = (int) ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) % ((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    // quit, close and deallocate channels 
    for (int i = 0; i < w; i++) {
        MESSAGE_TYPE m = QUIT_MSG;
        channels[i]->cwrite(&m, sizeof(MESSAGE_TYPE));
		delete channels[i];
    }


	// quit and close control channel


    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!" << endl;
    delete chan;

	// wait for server to exit
	wait(nullptr);
}