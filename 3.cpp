#include <iostream>
#include <mutex>
#include <chrono>
#include <thread>
#include <vector>

using namespace std;

class Forks { // вилки
    public:
    Forks(){}
    mutex mtx; // для каждой свой mutex
};

void eating(Forks& rightFork, Forks& leftFork, int number) { // философ ест
    unique_lock<mutex> rightLock(rightFork.mtx); // блокируем вилки
    unique_lock<mutex> leftLock(leftFork.mtx);
    cout << "Философ " << number << " ест\n";
    chrono::milliseconds timeout(2000); // время на поесть
    this_thread::sleep_for(timeout);
    cout << "Философ " << number << " доел\n";
}

int main() {
    const int philosophers = 5;
    vector<Forks> forks(philosophers); // вектор вилок
    vector<thread> threads; // вектор потоков
    cout << "Философ " << 1 << " думает\n"; // начинаем обход с 1 философа
    threads.emplace_back(eating, ref(forks[0]), ref(forks[philosophers - 1]), 1);
    for (int i = 1; i < philosophers; i++) {
        cout << "Философ " << i + 1 << " думает\n";
        threads.emplace_back(eating, ref(forks[i]), ref(forks[i - 1]), i + 1);
    }
    for (auto& t : threads) {
        t.join();
    }
    return 0;
}