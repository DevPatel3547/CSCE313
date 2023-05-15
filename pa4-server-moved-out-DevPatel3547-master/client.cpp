// your PA3 client code here
#include <fstream>
#include <iostream>
#include <thread>
#include <functional>
#include <cmath>
#include <string>
#include <cstdio>
#include <utility>  
#include <sys/time.h>
#include <sys/wait.h>
#include "BoundedBuffer.h"
#include "common.h"
#include "Histogram.h"
#include "HistogramCollection.h"
#include "TCPRequestChannel.h"


#define EGCNO 1
using namespace std;



struct PatientInfo
{
    int patientNumber;
    double responseData;
};

void worker_thread_function(BoundedBuffer *responseBuffer, BoundedBuffer *requestBuffer, TCPRequestChannel *chan, int bufSize)
{
    // Buffers
    char buffMsg[2048];
    char rcvBuffer[bufSize];
    double resp = 0;
    // Main loop
    while (true)
    {
        // Retrieve message from requestBuffer
        requestBuffer->pop(buffMsg, sizeof(buffMsg));
        MESSAGE_TYPE *msgType = reinterpret_cast<MESSAGE_TYPE *>(buffMsg);

        // Process file message
        if (*msgType == FILE_MSG)
        {
            filemsg *fileMessage = reinterpret_cast<filemsg *>(buffMsg);
            string fileName = reinterpret_cast<char *>(fileMessage + 1);
            int fileSize = sizeof(filemsg) + fileName.size() + 1;
            fileName = "received/" + fileName;

            // Send request and read response
            chan->cwrite(buffMsg, fileSize);
            chan->cread(rcvBuffer, bufSize);

            // Write response data to file
            FILE *outFile = fopen(fileName.c_str(), "r+");
            fseek(outFile, fileMessage->offset, SEEK_SET);
            fwrite(rcvBuffer, 1, fileMessage->length, outFile);
            fclose(outFile);
        }
        // Process data message
        else if (*msgType == DATA_MSG)
        {
            chan->cwrite(buffMsg, sizeof(datamsg));
            chan->cread(&resp, sizeof(double));

            // Prepare PatientInfo struct
            PatientInfo pInfo = {reinterpret_cast<datamsg *>(buffMsg)->person, resp};
            responseBuffer->push(reinterpret_cast<char *>(&pInfo), sizeof(pInfo));
        }
        // Process quit message
        else if (*msgType == QUIT_MSG)
        {
            chan->cwrite(msgType, sizeof(QUIT_MSG));
            break;
        }
    }
}

// Patient thread function with alternate approach
void patient_thread_function(BoundedBuffer *requestBuffer, int numOfRequests, int patientId)
{
    double initial_time = 0.0;
    
    // Iterate through all requests for the patient
    for (int i = 0; i < numOfRequests; ++i)
    {
        // Create the data message with patient ID, current time, and ECG number
        datamsg dataMsg(patientId, initial_time, 1);

        // Push the data message into the request buffer
        requestBuffer->push(reinterpret_cast<char *>(&dataMsg), sizeof(dataMsg));

        // Update the time for the next request
        initial_time += 0.004;
    }
}

// Histogram thread function with alternate approach
void histogram_thread_function(HistogramCollection *histCollection, BoundedBuffer *resp_buffer)
{
    bool continue_loop = true;

    // Loop until a quit message is received
    while (continue_loop)
    {
        // Initialize a buffer to hold the patient information
        char buffer[sizeof(PatientInfo)];

        // Pop the patient information from the response buffer
        resp_buffer->pop(buffer, sizeof(PatientInfo));

        // Convert the buffer to a PatientInfo struct
        PatientInfo patientInfo = *reinterpret_cast<PatientInfo *>(buffer);

        // Check for a quit message
        if (patientInfo.patientNumber == -1 && patientInfo.responseData == -1.0)
        {
            // Set flag to exit the loop
            continue_loop = false;
        }
        else
        {
            // Update the histogram with the patient data
            histCollection->update(patientInfo.patientNumber, patientInfo.responseData);
        }
    }
}




void file_thread_function(int fileSize, BoundedBuffer *requestBuffer, string fileName, int maxLength)
{
    int bufferLength = sizeof(filemsg) + fileName.size() + 1; // Calculate the buffer length
    int currentOffset = 0; // Initialize the current offset

    // Calculate the number of iterations, including the last part
    int numIterations = (fileSize + maxLength - 1) / maxLength;

    // Iterate through all parts of the file
    for (int i = 0; i < numIterations; i++)
    {
        // Determine the length for this iteration
        int currentLength = (i == numIterations - 1) ? fileSize % maxLength : maxLength;
        
        // Handle case when file size is a multiple of maxLength
        if (currentLength == 0) currentLength = maxLength;

        // Create a filemsg with the current offset and length
        filemsg msg(currentOffset, currentLength);

        // Create a temporary buffer as a vector
        vector<char> tempBuff(bufferLength);

        // Copy the filemsg into the buffer
        memcpy(tempBuff.data(), &msg, sizeof(filemsg));

        // Copy the file name into the buffer
        strcpy(tempBuff.data() + sizeof(filemsg), fileName.c_str());

        // Push the buffer into the request buffer
        requestBuffer->push(tempBuff.data(), bufferLength);

        // Update the current offset
        currentOffset += currentLength;
    }
}



int main(int argc, char *argv[])
{
    int n = 1000;        // default number of requests per "patient"
    int p = 10;          // number of patients [1:,15:]
    int w = 100;         // default number of worker threads
    int h = 20;          // default number of histogram threads
    int b = 20;          // default capacity of the request buffer (should be changed)
    int m = MAX_MESSAGE; // default capacity of the message buffer
    string f = "";       // name of file to be transferred

    string r; // add aditional arguments
    string a;

    // read arguments
    int opt;
    while ((opt = getopt(argc, argv, "n:p:w:h:b:m:f:a:r:")) != -1)
    {
        switch (opt)
        {
        case 'n':
            n = stoi(optarg);
            break;
        case 'p':
            p = stoi(optarg);
            break;
        case 'w':
            w = stoi(optarg);
            break;
        case 'h':
            h = stoi(optarg);
            break;
        case 'b':
            b = stoi(optarg);
            break;
        case 'm':
            m = stoi(optarg);
            break;
        case 'f':
            f = optarg;
            break;
        case 'a':
            a = optarg;
            break;
        case 'r':
            r = optarg;
            break;
        }
    }
    // Initialize TCPRequestChannel, BoundedBuffers, and HistogramCollection
    TCPRequestChannel *tcpChan = new TCPRequestChannel(a, r);
    BoundedBuffer reqBuf(b), respBuf(b);
    HistogramCollection histColl;

    // Create and add histograms to the collection
    for (int i = 0; i < p; ++i) {
        Histogram *hist = new Histogram(10, -2.0, 2.0);
        histColl.add(hist);
    }


    // Record start time
    struct timeval startTime, endTime;
    gettimeofday(&startTime, 0);

    // Thread creation
    thread fileThread[1];
    thread patientThreads[p];
    thread histThreads[h];

    // If no file is specified, create patient and histogram threads
    if (f.empty()) {
        for (int i = 0; i < p; ++i) {
            patientThreads[i] = thread(patient_thread_function, &reqBuf, n, i + 1);
        }
        for (int i = 0; i < h; ++i) {
            histThreads[i] = thread(histogram_thread_function, &histColl, &respBuf);
        }
    } else { // If a file is specified, create file thread
        string fileName = "received/" + f;
        FILE *outFile = fopen(fileName.c_str(), "w+");

        filemsg fileMsg(0, 0);
        int msgLen = sizeof(filemsg) + f.size() + 1;
        char buffer[msgLen];
        memcpy(buffer, &fileMsg, sizeof(filemsg));
        strcpy(buffer + sizeof(filemsg), f.c_str());
        tcpChan->cwrite(buffer, msgLen);

        __int64_t size;
        tcpChan->cread(&size, sizeof(__int64_t));

        fileThread[0] = thread(file_thread_function, size, &reqBuf, f, m);

        fclose(outFile);
    }

    vector<TCPRequestChannel *> tcpChannels;
    thread workerThreads[w];

    // Create and store worker threads and channels
    for (int i = 0; i < w; ++i) {
        TCPRequestChannel *channel = new TCPRequestChannel(a, r);
        tcpChannels.push_back(channel);
        workerThreads[i] = thread(worker_thread_function, &respBuf, &reqBuf, channel, m);
    }

    // Join threads
    if (f.empty()) {
        for (auto &thread : patientThreads) {
            thread.join();
        }
    } else {
        fileThread[0].join();
    }

    for (int i = 0; i < w; ++i) {
        MESSAGE_TYPE quitMsg = QUIT_MSG;
        reqBuf.push(reinterpret_cast<char *>(&quitMsg), sizeof(MESSAGE_TYPE));
    }

    for (auto &thread : workerThreads) {
        thread.join();
    }

    if (f.empty()) {
        for (int i = 0; i < h; ++i) {
            PatientInfo p{-1, -1};
            respBuf.push(reinterpret_cast<char *>(&p), sizeof(PatientInfo));
        }
        for (auto &thread : histThreads) {
            thread.join();
        }
    }

    // Record end time
    gettimeofday(&endTime, nullptr);

    // Print results
    if (f.empty()) {
        histColl.print();
    }

    int elapsedTime = 1e6 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec);
    int secs = elapsedTime / static_cast<int>(1e6);
    int usecs = elapsedTime % static_cast<int>(1e6);
    cout << "Time elapsed: " << secs << " seconds and " << usecs << " microseconds" << endl;

    // Terminate and deallocate channels
    for (auto &channel : tcpChannels) {
        MESSAGE_TYPE quitMsg = QUIT_MSG;
        channel->cwrite(reinterpret_cast<char *>(&quitMsg), sizeof(MESSAGE_TYPE));
        delete channel;
    }

    MESSAGE_TYPE quitMsg = QUIT_MSG;
    tcpChan->cwrite(reinterpret_cast<char *>(&quitMsg), sizeof(MESSAGE_TYPE));
    cout << "Over!" << endl;
    delete tcpChan;

    // Wait for server to exit
    wait(nullptr);


}