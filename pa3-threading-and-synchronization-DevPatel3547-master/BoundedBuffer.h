#ifndef _BOUNDEDBUFFER_H_
#define _BOUNDEDBUFFER_H_
#include <thread>         // std::thread
#include <mutex>          // std::mutex
#include <queue>
#include <vector>
#include <condition_variable> // std::condition_variable
#include <assert.h> 
#include <cstring> 
#include <iostream>
#include <string>
using std::string;
class BoundedBuffer {
private:


    // max number of items in the buffer
	int cap;

    /* The queue of items in the buffer
     * Note that each item a sequence of characters that is best represented by a vector<char> for 2 reasons:
	 *  1. An STL std::string cannot keep binary/non-printables
	 *  2. The other alternative is keeping a char* for the sequence and an integer length (b/c the items can be of variable length)
	 * While the second would work, it is clearly more tedious
     */
	

	// add necessary synchronization variables and data structures 
	//synch variables
	std::mutex m;
	std::condition_variable cv;

	//state variables 
	std::queue<std::vector<char>> q;


public:
	//BoundedBuffer (int _cap, string name);
	BoundedBuffer (int _cap);
	~BoundedBuffer ();

	void push (char* msg, int size);
	int pop (char* &msg, int size);
	void update();
	size_t size ();
	bool slot();
	bool data();
};

#endif
