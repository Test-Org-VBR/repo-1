#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <string>
#include <iomanip>
#include <random>
#include <chrono>

class BankAccount {
private:
    std::string accountNumber;
    double balance;
    std::mutex balanceMutex;
    std::atomic<int> transactionCount{0};

public:
    BankAccount(const std::string& accNum, double initialBalance)
        : accountNumber(accNum), balance(initialBalance) {}

    bool withdraw(double amount, const std::string& transactionId) {
        std::lock_guard<std::mutex> lock(balanceMutex);
        
        if (balance >= amount) {
            // Simulate some processing time
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            balance -= amount;
            transactionCount++;
            
            std::cout << "Transaction " << transactionId 
                      << ": Withdrew $" << std::fixed << std::setprecision(2) << amount 
                      << " from account " << accountNumber 
                      << ". New balance: $" << balance << std::endl;
            return true;
        } else {
            std::cout << "Transaction " << transactionId 
                      << ": Insufficient funds in account " << accountNumber 
                      << ". Current balance: $" << balance 
                      << ", Attempted withdrawal: $" << amount << std::endl;
            return false;
        }
    }

    bool deposit(double amount, const std::string& transactionId) {
        std::lock_guard<std::mutex> lock(balanceMutex);
        
        // Simulate some processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        balance += amount;
        transactionCount++;
        
        std::cout << "Transaction " << transactionId 
                  << ": Deposited $" << std::fixed << std::setprecision(2) << amount 
                  << " to account " << accountNumber 
                  << ". New balance: $" << balance << std::endl;
        return true;
    }

    bool transfer(BankAccount& destination, double amount, const std::string& transactionId) {
        // To prevent deadlocks, always lock accounts in the same order
        // We'll use the account number as the ordering criterion
        if (accountNumber < destination.accountNumber) {
            std::lock_guard<std::mutex> lockSrc(balanceMutex);
            std::lock_guard<std::mutex> lockDest(destination.balanceMutex);
            return executeTransfer(destination, amount, transactionId);
        } else if (accountNumber > destination.accountNumber) {
            std::lock_guard<std::mutex> lockDest(destination.balanceMutex);
            std::lock_guard<std::mutex> lockSrc(balanceMutex);
            return executeTransfer(destination, amount, transactionId);
        } else {
            // Same account, just use one lock
            std::lock_guard<std::mutex> lock(balanceMutex);
            std::cout << "Transaction " << transactionId 
                      << ": Cannot transfer to the same account." << std::endl;
            return false;
        }
    }

private:
    bool executeTransfer(BankAccount& destination, double amount, const std::string& transactionId) {
        if (balance >= amount) {
            // Simulate some processing time
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            
            balance -= amount;
            destination.balance += amount;
            transactionCount++;
            destination.transactionCount++;
            
            std::cout << "Transaction " << transactionId 
                      << ": Transferred $" << std::fixed << std::setprecision(2) << amount 
                      << " from account " << accountNumber 
                      << " to account " << destination.accountNumber 
                      << ". New balances: $" << balance 
                      << " (source), $" << destination.balance 
                      << " (destination)" << std::endl;
            return true;
        } else {
            std::cout << "Transaction " << transactionId 
                      << ": Insufficient funds for transfer from account " << accountNumber 
                      << ". Current balance: $" << balance 
                      << ", Attempted transfer: $" << amount << std::endl;
            return false;
        }
    }
};

// Function to simulate random transactions
void performRandomTransactions(BankAccount& account1, BankAccount& account2, int numTransactions) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> transactionType(0, 2); // 0: withdraw, 1: deposit, 2: transfer
    std::uniform_real_distribution<> amountDist(10.0, 200.0);
    
    for (int i = 0; i < numTransactions; ++i) {
        std::string transactionId = "T" + std::to_string(i+1);
        double amount = std::round(amountDist(gen) * 100) / 100; // Round to 2 decimal places
        
        switch (transactionType(gen)) {
            case 0: // Withdraw
                account1.withdraw(amount, transactionId);
                break;
            case 1: // Deposit
                account1.deposit(amount, transactionId);
                break;
            case 2: // Transfer
                account1.transfer(account2, amount, transactionId);
                break;
        }
        
        // Small delay between transactions
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

int main() {
    // Create two bank accounts
    BankAccount account1("ACC001", 1000.0);
    BankAccount account2("ACC002", 1500.0);
    
    std::cout << "Starting concurrent bank transactions simulation..." << std::endl;
    
    // Create threads to perform concurrent transactions
    std::vector<std::thread> threads;
    const int numThreads = 4;
    const int transactionsPerThread = 5;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(performRandomTransactions, 
                            std::ref(account1), 
                            std::ref(account2), 
                            transactionsPerThread);
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "All transactions completed." << std::endl;
    
    return 0;
}