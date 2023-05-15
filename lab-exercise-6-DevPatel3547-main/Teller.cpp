#include <string>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>
#include "BankAccount.h"
using namespace std;

// Helper function for printing the results of each approach
void print_helper(int _approach_num, const chrono::time_point<chrono::steady_clock>& start,
    const chrono::time_point<chrono::steady_clock>& end, BankAccount& a);

// Thread function for processing a transaction without mutex
void teller_thread(BankAccount* b, int _amount) {
    b->transaction(_amount);
}

// Thread function for processing a transaction with mutex (thread-safe)
void teller_threadsafe(BankAccount* c, int _amount) {
    c->transaction_threadsafe(_amount);
}

int main(int argc, char* argv[]) {
    string fname = "transactions.csv";
    int num_trans = 2000;

    // Parse command-line arguments for input file
    int opt;
    while ((opt = getopt(argc, argv, "i:")) != -1) {
        switch (opt) {
            case 'i':
                fname = optarg;
                break;
        }
    }

    // Read input file and store transactions in an array
    ifstream in_file(fname);
    if (!in_file.is_open()) {
        perror("ifstream");
        exit(EXIT_FAILURE);
    }
    int* trans_arr = new int[num_trans]; 
    for (int i = 0; i < num_trans; ++i) {
        in_file >> trans_arr[i];
    }
    in_file.close();

    // Approach 1: Synchronous transactions
    BankAccount A;
    const chrono::time_point<chrono::steady_clock> start1 = chrono::steady_clock::now();
    for (int i = 0; i < num_trans; ++i) {
        A.transaction(trans_arr[i]);
    }
    const auto end1 = chrono::steady_clock::now();
    print_helper(1, start1, end1, A);

    // Approach 2: Transactions using threads without mutex
    BankAccount B;
    vector<thread> t_vec;
    const chrono::time_point<chrono::steady_clock> start2 = chrono::steady_clock::now();
    for (int i = 0; i < num_trans; ++i) {
        t_vec.push_back(thread(teller_thread, &B, trans_arr[i]));
    }
    for (size_t i = 0; i < t_vec.size(); ++i) {
        t_vec.at(i).join();
    }
    const auto end2 = chrono::steady_clock::now();
    print_helper(2, start2, end2, B);

    // Approach 3: Transactions using threads with mutex (thread-safe)
    BankAccount C;
    vector<thread> t_vec2;
    const chrono::time_point<chrono::steady_clock> start3 = chrono::steady_clock::now();
    for (int i = 0; i < num_trans; ++i) {
        t_vec2.push_back(thread(teller_threadsafe, &C, trans_arr[i]));
    }
    for (size_t i = 0; i < t_vec2.size(); ++i) {
        t_vec2.at(i).join();
    }
    const auto end3 = chrono::steady_clock::now();
    print_helper(3, start3, end3, C);

    delete[] trans_arr;
}

void print_helper(int _approach_num, const chrono::time_point<chrono::steady_clock>& start, 
    const chrono::time_point<chrono::steady_clock>& end, BankAccount& a) {
    cout << "Approach " << _approach_num << " took " << (end - start) / 1ms;
    cout << "ms to achieve a final balance of ";
    a.print_balance(); cout << endl;
}
