#include <iostream>
#include <random>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <semaphore>
#include <condition_variable>

using namespace std;

class SemaphoreSlim {
    public:
    SemaphoreSlim(int initialCount, int maxCount) : iCount(initialCount), mCount(maxCount) {}
    void acquire() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]() {
            return iCount > 0;
        });
        --iCount; 
    }

    void release() {
        lock_guard<mutex> lock(mtx);
        if (iCount < mCount) {
            ++iCount;
            cv.notify_one();
        }
    }

    private:
    mutex mtx;
    condition_variable cv;
    int iCount, mCount;
};

void randomSymbols(char& symbol) { // генерация рандомных символов ASCII
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(32, 126);
    symbol = static_cast<char>(dis(gen));
}

void threadMutex(char& symbol, mutex& mtx, vector<char>& allSymbols) { // mutex
    for (int i = 0; i < 1000; i++) { 
        randomSymbols(symbol);
        lock_guard<mutex> lock(mtx); // только один поток может получить доступ к вектору
        allSymbols.push_back(symbol);
    }
}

// void threadSemaphore(char& symbol, counting_semaphore<4>& sem, vector<char>& allSymbols) { // semaphore
//     for (int i = 0; i < 1000; i++) {
//         randomSymbols(symbol);
//         sem.acquire();
//         allSymbols.push_back(symbol);
//         sem.release();
//     }
// }

void threadSemaphoreSlim(char& symbol, SemaphoreSlim& semSlim, vector<char>& allSymbols) {
    for (int i = 0; i < 1000; i++) {
        randomSymbols(symbol);
        semSlim.acquire();
        allSymbols.push_back(symbol);
        semSlim.release();
    }
}

int main() {
    vector<char> allSymbols; // вектор символов
    mutex mtx; 
    char symbol; // символ

    auto start = chrono::high_resolution_clock::now(); // измерение начала - запуск таймера
    vector<thread> threads; // вектор потоков
    for (int i = 0; i < 4; i++) { // параллельный запуск 4 потоков
        threads.emplace_back(threadMutex, ref(symbol), ref(mtx), ref(allSymbols)); // ref - ссылки
    }
    for (auto& t : threads) {
        t.join(); // блокирует выполнение основного потока
    }
    auto end = chrono::high_resolution_clock::now(); // измерение конца
    chrono::duration<double> elapsed = end - start; // вычисляем продолжительность 
    cout << "Mutex time: " << elapsed.count() << " seconds\n";
    threads.clear();
    allSymbols.clear();

    // counting_semaphore<4> sem(4);
    // start = chrono::high_resolution_clock::now();
    // for (int i = 0; i < 4; i++) {
    //     threads.emplace_back(threadSemaphore, ref(symbol), ref(sem), ref(allSymbols));
    // }
    // for (auto& t : threads) {
    //     t.join();
    // }
    // end = chrono::high_resolution_clock::now();
    // elapsed = end - start;
    // cout << "Semaphore time: " << elapsed.count() << " seconds\n";
    // threads.clear();
    // allSymbols.clear();

    SemaphoreSlim semSlim(4, 4);
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadSemaphoreSlim, ref(symbol), ref(semSlim), ref(allSymbols));
    }
    for (auto& t : threads) {
        t.join();
    }
    end = chrono::high_resolution_clock::now();
    elapsed = end - start;
    cout << "SemaphoreSlim time: " << elapsed.count() << " seconds\n";
    threads.clear();
    allSymbols.clear();
    return 0;
}