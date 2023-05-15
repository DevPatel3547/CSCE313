#include <vector> // vector, push_back, at
#include <string> // string
#include <iostream> // cin, getline
#include <fstream> // ofstream
#include <unistd.h> // getopt, exit, EXIT_FAILURE
#include <assert.h> // assert
#include <thread> // thread, join
#include <sstream> // stringstream

#include "BoundedBuffer.h" // BoundedBuffer class

#define MAX_MSG_LEN 256

using namespace std;

/************** Helper Function Declarations **************/

void parse_column_names(vector<string>& _colnames, const string& _opt_input);
void write_to_file(const string& _filename, const string& _text, bool _first_input=false);

/************** Thread Function Definitions **************/

// "primary thread will be a UI data entry point"
void ui_thread_function(BoundedBuffer* bb) {
    string input;
    while (true) {
        cout << "Enter data (or type 'Exit' to quit): ";
        getline(cin, input);

        if (input == "Exit") {
            bb->push(const_cast<char*>(input.c_str()), input.size());
            break;
        }

        bb->push(const_cast<char*>(input.c_str()), input.size());
    }
}

// "second thread will be the data processing thread"
// "will open, write to, and close a csv file"
void data_thread_function(BoundedBuffer* bb, string filename, const vector<string>& colnames) {
    // Write column names to the file
    string colnames_str;
    for (const auto& colname : colnames) {
        colnames_str += colname + ",";
    }
    colnames_str.pop_back(); // remove the trailing comma
    colnames_str += "\n";
    write_to_file(filename, colnames_str, true);

    // Process data from the BoundedBuffer
    char msg[MAX_MSG_LEN];
    while (true) {
        int size = bb->pop(msg, MAX_MSG_LEN);
        string data(msg, msg + size);

        if (data == "Exit") {
            break;
        }

        write_to_file(filename, data + "\n");
    }
}

/************** Main Function **************/

int main(int argc, char* argv[]) {
    vector<string> colnames;
    string fname;
    BoundedBuffer* bb = new BoundedBuffer(3);

    int opt;
    while ((opt = getopt(argc, argv, "c:f:")) != -1) {
        switch (opt) {
            case 'c':
                parse_column_names(colnames, optarg);
                break;
            case 'f':
                fname = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [-c colnames] [-f filename]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Instantiate ui and data threads
    thread ui_thread(ui_thread_function, bb);
    thread data_thread(data_thread_function, bb, fname, colnames);

    // Join ui_thread
    ui_thread.join();

    // Join data_thread
    data_thread.join();

    // Cleanup: delete members on heap
    delete bb;
}

/************** Helper Function Definitions **************/

// function to parse column names into vector
// input: _colnames (vector of column name strings), _opt_input(input from optarg for -c)
void parse_column_names(vector<string>& _colnames, const string& _opt_input) {
    stringstream sstream(_opt_input);
    string tmp;
    while (sstream >> tmp) {
        _colnames.push_back(tmp);
    }
}

// function to append "text" to end of file
// input: filename (name of file), text (text to add to file), first_input (whether or not this is the first input of the file)
void write_to_file(const string& _filename, const string& _text, bool _first_input) {
    // based on https://stackoverflow.com/questions/26084885/appending-to-a-file-with-ofstream
    // open file to either append or clear file
    ofstream ofile;
    if (_first_input)
        ofile.open(_filename);
    else
        ofile.open(_filename, ofstream::app);
    if (!ofile.is_open()) {
        perror("ofstream open");
        exit(-1);
    }

    // sleep for a random period up to 5 seconds
    usleep(rand() % 5000);

    // add data to csv
    ofile << _text;

    // close file
    ofile.close();
}