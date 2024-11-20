#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <thread>

using namespace std;

class Staffer { // структура работника
    public:
    string name;
    string post;
    int office;
    int salary;
};

vector<Staffer> generateWorker() { // генерация работников
    vector<string> names = {"Анна Смирнова", "Дмитрий Иванов", "Екатерина Петрова", "Алексей Волков", "Мария Сергеева", "Илья Фёдоров",
    "Виктория Соловьёва", "Андрей Чернов", "Наталья Кузнецова", "Сергей Никитин"};
    vector<string> posts = {"Менеджер по продажам", "Бухгалтер", "Специалист по маркетингу", "Программист", "HR-менеджер",
    "Аналитик данных", "Дизайнер", "Менеджер проектов", "Клиентский менеджер", "ИТ-администратор"};
    vector<int> salarys = {70'000, 60'000, 65'000, 90'000, 55'000, 80'000, 70'000, 75'000, 65'000, 70'000};

    random_device rd;
    mt19937 gen(rd());
    shuffle(names.begin(), names.end(), gen); // перемешивание элементов вектора
    shuffle(posts.begin(), posts.end(), gen);
    shuffle(salarys.begin(), salarys.end(), gen);

    vector<Staffer> workers; // заполнение вектора работников
    for (int i = 0; i < 10; i++) {
        Staffer staffer;
        staffer.name = names[i];
        staffer.post = posts[i];
        staffer.office = rd() % (3) + 1;
        staffer.salary = salarys[i];
        workers.push_back(staffer);
    }
    return workers;
}

// подсчёт средней зарплаты
void calculateSum(const vector<Staffer>& workers, int i, int sum, int count, vector<int>& sums, mutex& mtx) {
    for (auto& worker : workers) {
            if (worker.office == i) {
                sum += worker.salary;
                count++;
            }
        }
    lock_guard<mutex> lock(mtx);
    sums[i - 1] = (sum / count);
    sum = 0;
    count = 0;
}

void printWorkers(const vector<Staffer>& workers, int i, const vector<int>& sums, mutex& mtx) { // вывод работников
    for (auto& worker : workers) {
        if (worker.office == i + 1 && worker.salary > sums[i]) {
            lock_guard<mutex> lock(mtx);
            cout << "Имя: " << worker.name << "\tдолжность: " << worker.post << "\tотдел: " << worker.office << "\tзарплата: " << worker.salary << endl;
        }
    }
}

int main() {
    vector<Staffer> workers = generateWorker(); // вектор работников
    for (auto& worker : workers) {
        cout << "Имя: " << worker.name << "\tдолжность: " << worker.post << "\tотдел: " << worker.office << "\tзарплата: "
        << worker.salary << endl;
    }
    vector<int> sums; // вектор средний зарплат по отделам
    int sum = 0, count = 0;

    // однопоточная обработка
    auto start = chrono::high_resolution_clock::now();
    for (int i = 1; i <= 3; i++) { // считаем среднюю зарплату по отделам
        for (auto& worker : workers) {
            if (worker.office == i) {
                sum += worker.salary;
                count++;
            }
        }
        sums.push_back(sum / count);
        sum = 0;
        count = 0;
    }
    cout << endl;
    for (int i = 0; i < sums.size(); i++) {
        cout << "Средняя зарплата по " << i + 1 << " отделу: " << sums[i] << endl;
    }
    cout << endl;
    for (int i = 0; i < sums.size(); i++) { // ищем работников с зарплатой выше среднего
        for (auto& worker : workers) {
            if (worker.office == i + 1 && worker.salary > sums[i]) {
                cout << "Имя: " << worker.name << "\tдолжность: " << worker.post << "\tотдел: " << worker.office << "\tзарплата: " << worker.salary << endl;
            }
        }
    }
    sums = {0, 0, 0};
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;
    cout << "\nВремя выполнения однопоточной обработки: " << elapsed.count() << endl;

    // многопоточная обработка
    mutex mtx;
    vector<thread> threads;
    start = chrono::high_resolution_clock::now();
    for (int i = 1; i <= 3; i++) { // многопоточный подсчёт средней зарплаты
        threads.emplace_back(calculateSum, ref(workers), i, sum, count, ref(sums), ref(mtx));
    }
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();
    cout << endl;
    for (int i = 0; i < sums.size(); i++) { 
        cout << "Средняя зарплата по " << i + 1 << " отделу: " << sums[i] << endl;
    }
    cout << endl;

    for (int i = 0; i < sums.size(); i++) { // многопоточный вывод работников
        threads.emplace_back(printWorkers, ref(workers), i, ref(sums), ref(mtx));
    }
    for (auto& t : threads) {
        t.join();
    }
    end = chrono::high_resolution_clock::now();
    elapsed = end - start;
    cout << "\nВремя выполнения многопоточной обработки: " << elapsed.count() << endl;
    return 0;
}