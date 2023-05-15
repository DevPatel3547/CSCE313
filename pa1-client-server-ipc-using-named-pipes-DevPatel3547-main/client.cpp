/*
	Original author of the Starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Dev RahulBhai Patel
	UIN: 931007975
	Date: 02/09/2023
*/
#include <bits/stdc++.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;
using namespace std::chrono;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	int m = MAX_MESSAGE;
	string filename = "";
	char* cbuf;
	bool isnewChannel = false;
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				m = atoi(optarg);
				cbuf = optarg;
				break;
			case 'c':
				isnewChannel = true;
				break;
		}
	}

	vector<FIFORequestChannel*> channels;

	if(fork()==0){
		if(m!=MAX_MESSAGE){


			char* serverArg[] = {(char*) "./server", (char*) "-m",cbuf,NULL};
			execvp(serverArg[0],serverArg);
		}else{


			char* serverArg[] = {(char*) "./server",NULL};
			execvp(serverArg[0],serverArg); 
		}


	}else{
		FIFORequestChannel contChan("control", FIFORequestChannel::CLIENT_SIDE);

		FIFORequestChannel chan = contChan;

		if(isnewChannel){


			MESSAGE_TYPE newChan = NEWCHANNEL_MSG;
			contChan.cwrite(&newChan , sizeof(MESSAGE_TYPE));
			char nameb[50];
			contChan.cread(nameb,sizeof(nameb));

			FIFORequestChannel* newChannel = new FIFORequestChannel{nameb,FIFORequestChannel::CLIENT_SIDE};


			channels.push_back(newChannel);
			chan = *newChannel;

		}
		
;

// Check if the user specified an ECG and a time
if(p>0 && t>0 && e>0){
	cout << "Requesting ECG " << e << " data for person " << p << " at time " << t << endl;

	// Create data message and send it to the server
	datamsg d(p, t, e);
	char buf[MAX_MESSAGE];
	memcpy(buf, &d, sizeof(datamsg));
	chan.cwrite(buf, sizeof(datamsg));

	double ecgValue;
	// Wait for server response and output ECG value to console
	chan.cread(&ecgValue, sizeof(double));
	cout << "ECG " << e << " value: " << ecgValue << endl;

} else if(p>0 && t==-1 && e==-1){
	cout << "Getting the first 1000 data points for person " << p << endl;	

	// Open file to write ECG data
	ofstream ecgData;
	
	ecgData.open("received/x1.csv");

			for(double currTime=0;currTime<4;currTime+=0.004){
				ecgData << currTime << ",";
				
				char buf[MAX_MESSAGE]; 
			    	datamsg x1(p, currTime, 1);
				memcpy(buf, &x1, sizeof(datamsg));

				chan.cwrite(buf, sizeof(datamsg));
				double ecgVal1;

				chan.cread(&ecgVal1, sizeof(double));

				ecgData << ecgVal1 << ",";
				
				datamsg x2(p, currTime, 2);
				memcpy(buf, &x2, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg));
				double ecgVal2;
				chan.cread(&ecgVal2, sizeof(double));
				
				if(currTime==3.996){
					ecgData << ecgVal2;
				}else{
					ecgData << ecgVal2 << endl;
				}
			}
			ecgData.close();
		}else if(filename!=""){
			// sending a non-sense message, you need to change this

			auto Start = high_resolution_clock::now();//calculate time of the function
			filemsg fm(0, 0);	
			int len = sizeof(filemsg) + (filename.size() + 1);
			
			char* rb = new char[len];
			char* retBuf = new char[m];

			memcpy(rb, &fm, sizeof(filemsg));
			strcpy(rb + sizeof(filemsg), filename.c_str());
			chan.cwrite(rb, len);  
			
			int64_t filesize =0;
			chan.cread(&filesize,sizeof(int64_t));


			int reqNum = filesize/m;
			double actualReqNum = double(filesize)/double(m);
			cout << "Number of request to be sent: " << reqNum << ", Actual: " << actualReqNum << endl;
			
			string fp = "received/" + string(filename);
			FILE* outFile = fopen(fp.c_str(),"wb");

			if(filesize<m){

				filemsg* file_req = (filemsg*) rb;


				file_req->offset =0;
				file_req->length = filesize;

				chan.cwrite(rb,len);


				chan.cread(retBuf,m);
				fwrite(retBuf,1,file_req->length,outFile);
		
			}else{

				filemsg* file_req = (filemsg*) rb;
				file_req->offset = 0;
				file_req->length = m;
			
				chan.cwrite(rb,len);
				chan.cread(retBuf,m);


				fwrite(retBuf,1,file_req->length,outFile);

				while(--reqNum){
					file_req->offset += m;
					chan.cwrite(rb,len);


					chan.cread(retBuf,m);
					fwrite(retBuf,1,file_req->length,outFile);	
				}
				file_req->offset +=m;
				//if the requested offset is not equal to the filesize
				if(file_req->offset != filesize){

					int lastSector = filesize - file_req->offset; 
					file_req->length = lastSector;
					chan.cwrite(rb,len);


					chan.cread(retBuf,m);
					fwrite(retBuf,1,file_req->length,outFile);
				}
			}
			
			auto stop = high_resolution_clock::now();// stop time => Hence, the total time of the function will be stop - start
			//total duration
			auto duration = duration_cast<microseconds>(stop - Start);
			
			int micro = duration.count();
			double seconds = double(micro)/double(1000000);
			
			cout << "Time take: " << micro << " microseconds, "; 
			cout << seconds << setprecision(6);

		    cout << " seconds" << endl;	
			
			
			delete[] rb;
			delete[] retBuf;
		}


		if(isnewChannel){

			for(size_t i=0;i<channels.size();i++){
				delete channels[i];
			}
		}


		MESSAGE_TYPE m = QUIT_MSG;
		chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	}
	return 0;
}

