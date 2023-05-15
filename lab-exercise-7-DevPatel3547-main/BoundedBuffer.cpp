#include "BoundedBuffer.h"
#include <mutex>
#include <condition_variable>
#include <cassert>
using namespace std;


BoundedBuffer::BoundedBuffer (int _cap) : cap(_cap) {
    // modify as needed
}

BoundedBuffer::~BoundedBuffer () {
    // modify as needed
}

void BoundedBuffer::push (char* msg, int size) {
    unique_lock<mutex> lock(mtx);

    // Wait until there is room in the queue (i.e., queue length is less than cap)
    not_full.wait(lock, [this] { return q.size() < cap; });

    // Convert the incoming byte sequence given by msg and size into a vector<char>
    vector<char> data(msg, msg + size);

    // Push the vector at the end of the queue
    q.push(data);

    // Wake up threads that were waiting for push
    not_empty.notify_one();
}

int BoundedBuffer::pop (char* msg, int size) {
    unique_lock<mutex> lock(mtx);

    // Wait until the queue has at least 1 item
    not_empty.wait(lock, [this] { return !q.empty(); });

    // Pop the front item of the queue. The popped item is a vector<char>
    vector<char> data = q.front();
    q.pop();

    // Convert the popped vector<char> into a char*, copy that into msg;
    // assert that the vector<char>'s length is <= size
    assert(data.size() <= size);
    copy(data.begin(), data.end(), msg);

    // Wake up threads that were waiting for pop
    not_full.notify_one();

    // Return the vector's length to the caller so that they know how many bytes were popped
    return data.size();
}

size_t BoundedBuffer::size () {
    return q.size();
}
