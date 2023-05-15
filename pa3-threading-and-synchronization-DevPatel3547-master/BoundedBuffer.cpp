#include "BoundedBuffer.h"

using namespace std;

BoundedBuffer::BoundedBuffer (int capacity) : cap(capacity) {
}

BoundedBuffer::~BoundedBuffer () {
}

void BoundedBuffer::push(char* message, int len) {
    std::vector<char> charVector(len); // Initialize vector with given length

    for (int i = 0; i < len; i++) {
        charVector[i] = message[i]; // Copy input message to local vector
    }

    {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [this] { return static_cast<int>(q.size()) < (cap + 1); });

        q.push(std::move(charVector)); // Move local vector to queue

        cv.notify_all();
    }
}

int BoundedBuffer::pop(char*& message, int len) {
    std::vector<char> charVector;

    {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [this] { return static_cast<int>(q.size()) >= 1; });

        charVector = std::move(q.front()); // Move front element of queue to local vector
        q.pop(); // Remove front element from queue

        cv.notify_all();
    }

    // Ensure local vector size is within given limit
    assert(static_cast<int>(charVector.size()) <= len);

    // Copy local vector data to output message
    std::memcpy(message, charVector.data(), static_cast<int>(charVector.size()));

    return static_cast<int>(charVector.size());
}

bool BoundedBuffer::slot() {
    return (int(q.size()) < (cap + 1));
}

bool BoundedBuffer::data() {
    return (int(q.size()) >= 1);
}

size_t BoundedBuffer::size () {
    return q.size();
}
