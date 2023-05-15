#include "BankAccount.h"

// interface function to perform *threadsafe* transactions
void BankAccount::transaction_threadsafe(int _amount) {
    // LOCK other threads from using transaction() at the same time
    std::unique_lock<std::mutex> lock(m);
    transaction(_amount);
    // UNLOCK for the next thread to access transaction() (automatically unlocked by unique_lock destructor)
}

/***************** ALL FUNCTIONS BELOW THIS LINE ARE COMPLETE ****************/

BankAccount::BankAccount() : balance(0) {}

void BankAccount::transaction(int _amount) {
    int temp = balance;
    temp += _amount;
    usleep(rand() % 50);
    balance = temp;
}

void BankAccount::print_balance() {
    std::string currency = this->balance < 0 ? "-$" : "$";
    std::cout << currency << abs(this->balance);
}
