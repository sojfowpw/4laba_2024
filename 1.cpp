#include <iostream>
#include <random>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <semaphore.h>
#include <condition_variable>
#include <atomic>

using namespace std;

class Semaphore { // реализация semaphore
    public:
    Semaphore(int initialCount, int maxCount) : iCount(initialCount), mCount(maxCount) {
        sem_init(&sem, 0, initialCount);
    }
    void acquire() { // захватывает семафор
        sem_wait(&sem); // sem--
    }

    void release() { // освобождает семафор
        sem_post(&sem); // sem++
    }

    private:
    sem_t sem;
    int iCount, mCount;
};

class SemaphoreSlim { // реализация semaphoreSlim
    public:
    SemaphoreSlim(int initialCount, int maxCount) : iCount(initialCount), mCount(maxCount) {} // конструктор
    void acquire() { // захват семафора, блокируя поток, если семафор уже захвачен другими потоками
        unique_lock<mutex> lock(mtx); // защита доступа к счётчику
        cv.wait(lock, [this]() { // ожидает, пока счётчик не станет > 0, поток блокируется, пока другой не вызовет release()
            return iCount > 0;
        });
        --iCount; // уменьшаем счётчик, если семафор захвачен потоком
    }

    void release() { // освобождает семафор
        lock_guard<mutex> lock(mtx); // защита доступа к счётчику
        if (iCount < mCount) {
            ++iCount;
            cv.notify_one(); // уведомление одного из ожидающих потоков, что семафор свободен
        }
    }

    private:
    mutex mtx;
    condition_variable cv; // позволяет потокам ожидать наступления определённого условия
    int iCount, mCount; // начальное и максимальное значение счётчиков семафора
};

class Barrier { // реализация Barrier 
    public:
    Barrier(int count) : initialCount(count), maxCount(count), generationCount(0) {} // конструктор
    void wait() { // ожидает, пока все потоки не достигнут барьера
        unique_lock<mutex> lock(mtx); // защита доступа переменных
        int initialGen = generationCount; // текущее поколение
        if (--initialCount == 0) { // если счётчик 0 - все потоки достигли барьера
            generationCount++; // увеличиваем счётчик поколений, чтобы уведомить потоки о барьере
            initialCount = maxCount; // сбрасываем счётчик
            cv.notify_all(); // уведомляем все ожидающие потоки
        }
        else {
            cv.wait(lock, [this, initialGen] { // ожидает, пока поколение не изменится
                return initialGen != generationCount;
            });
        }
    }

    private:
    mutex mtx;
    condition_variable cv;
    int initialCount, maxCount, generationCount; // текущий счётчик, максимальное число потоков,
    // счётчик поколоений для уведомления потоков, что барьер потоков достигнут
};

class Monitor { // реализация monitor
    public:
    Monitor() : isReady(false) {}
    void locker() { // захват 
        unique_lock<mutex> lock(mtx); // защита доступа к переменным
        while(isReady) { // поток блокируется если, монитор захвачен
            cv.wait(lock); // блокировка текущего потока и освобождает mutex, чтобы другие потоки могли получить доступ
        }
        isReady = true;
    }

    void unlocker() { // освобождение
        lock_guard<mutex> lock(mtx);
        isReady = false; // монитор освобождён
        cv.notify_one(); // уведомляет один из ожидающих потоков
    }

    private:
    mutex mtx;
    condition_variable cv;
    bool isReady;
};

void randomSymbols(char& symbol) { // генерация рандомных символов ASCII
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(32, 126);
    symbol = static_cast<char>(dis(gen));
}

void threadMutex(char& symbol, mutex& mtx, vector<char>& allSymbols) { // mutex
    for (int i = 0; i < 10000; i++) { 
        randomSymbols(symbol);
        lock_guard<mutex> lock(mtx); // только один поток может получить доступ к вектору
        allSymbols.push_back(symbol);
    }
}

void threadSemaphore(char& symbol, Semaphore& sem, vector<char>& allSymbols) { // semaphore
    for (int i = 0; i < 10000; i++) {
        randomSymbols(symbol);
        sem.acquire();
        allSymbols.push_back(symbol);
        sem.release();
    }
}

void threadSemaphoreSlim(char& symbol, SemaphoreSlim& semSlim, vector<char>& allSymbols) { // semaphoreSlim
    for (int i = 0; i < 10000; i++) {
        randomSymbols(symbol);
        semSlim.acquire(); // захват семафора и блокировка потока
        allSymbols.push_back(symbol);
        semSlim.release(); // освобождение семафора
    }
}

void threadBarrier(char& symbol, Barrier& barrier, vector<char>& allSymbols) { // barrier
    for (int i = 0; i < 10000; i++) {
        randomSymbols(symbol);
        barrier.wait(); // ожидаем, пока все потоки не достигнут барьера
        allSymbols.push_back(symbol);
    }
}

void threadSpinLock(char& symbol, atomic_flag& spinLock, vector<char>& allSymbols) { // spinLock
    for (int i = 0; i < 10000; i++) {
        randomSymbols(symbol);
        while (spinLock.test_and_set(memory_order_acquire)) {} // устанавливаем флаг, поток входит в режим ожидания
        allSymbols.push_back(symbol);
        spinLock.clear(memory_order_release); // сбрасываем флаг, освобождая спинлок
    }
}

void threadSpinWait(char& symbol, atomic_flag& spinLock, vector<char>& allSymbols) { // spinWait
    for (int i = 0; i < 10000; i++) {
        randomSymbols(symbol);
        while (spinLock.test_and_set(memory_order_acquire)) { // если флаг установлен
            this_thread::yield(); // передаём управление другим потокам, готовым к выполнению
        }
        allSymbols.push_back(symbol);
        spinLock.clear(memory_order_release);
    }
}

void threadMonitor(char& symbol, Monitor& monitor, vector<char>& allSymbols) { // monitor
    for (int i = 0; i < 10000; i++) {
        randomSymbols(symbol);
        monitor.locker(); // захват 
        allSymbols.push_back(symbol);
        monitor.unlocker(); // освобождение
    }
}

int main() {
    vector<char> allSymbols; // вектор символов
    mutex mtx; 
    char symbol; // символ
    
    // mutex
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

    // semaphore
    Semaphore sem(4, 4);
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadSemaphore, ref(symbol), ref(sem), ref(allSymbols));
    }
    for (auto& t : threads) {
        t.join();
    }
    end = chrono::high_resolution_clock::now();
    elapsed = end - start;
    cout << "Semaphore time: " << elapsed.count() << " seconds\n";
    threads.clear();
    allSymbols.clear();

    // semaphoreSlim
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

    // barrier
    Barrier barrier(4);
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadBarrier, ref(symbol), ref(barrier), ref(allSymbols));
    }
    for (auto& t : threads) {
        t.join();
    }
    end = chrono::high_resolution_clock::now();
    elapsed = end - start;
    cout << "Barrier time: " << elapsed.count() << " seconds\n";
    threads.clear();
    allSymbols.clear();

    // spinLock
    atomic_flag spinLock = ATOMIC_FLAG_INIT; // переменная - флаг в неустановленном состоянии
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadSpinLock, ref(symbol), ref(spinLock), ref(allSymbols));
    }
    for (auto& t : threads) {
        t.join();
    }
    end = chrono::high_resolution_clock::now();
    elapsed = end - start;
    cout << "SpinLock time: " << elapsed.count() << " seconds\n";
    threads.clear();
    allSymbols.clear();

    // spinWait
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadSpinWait, ref(symbol), ref(spinLock), ref(allSymbols));
    }
    for (auto& t : threads) {
        t.join();
    }
    end = chrono::high_resolution_clock::now();
    elapsed = end - start;
    cout << "SpinWait time: " << elapsed.count() << " seconds\n";
    threads.clear();
    allSymbols.clear();

    // monitor
    Monitor monitor;
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(threadMonitor, ref(symbol), ref(monitor), ref(allSymbols));
    }
    for (auto& t : threads) {
        t.join();
    }
    end = chrono::high_resolution_clock::now();
    elapsed = end - start;
    cout << "Monitor time: " << elapsed.count() << " seconds\n";
    return 0;
}