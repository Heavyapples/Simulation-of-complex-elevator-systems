#include <iostream>
#include <vector>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <limits.h>
#include <random>

#define MAX_CAPACITY 18
#define ELEVATORS 10
#define FLOORS 40
#define PASSENGERS 100

enum Direction { UP, DOWN, IDLE };
enum Status { WAITING, IN_ELEVATOR, ARRIVED };

class Passenger {
public:
    int wait_start_time; // 乘客开始等待的时间
    int total_wait_time; // 乘客的总等待时间
    int id;
    int current_floor;
    int target_floor;
    int arrival_time;
    int rides_left;
    int wait_time;
    Status status;
    bool has_requested_elevator;

    Passenger(int id, int current_floor, int target_floor, int arrival_time, int rides_left, int wait_time) : id(id), current_floor(current_floor), target_floor(target_floor), arrival_time(arrival_time), rides_left(rides_left), wait_time(wait_time), status(WAITING), has_requested_elevator(false), wait_start_time(-1), total_wait_time(0) {}

    // 乘客开始等待时更新开始等待的时间
    void start_waiting(int current_time) {
        wait_start_time = current_time;
    }

    // 乘客结束等待时计算等待时间并更新总等待时间
    void end_waiting(int current_time) {
        if (wait_start_time != -1) {
            total_wait_time += current_time - wait_start_time;
            wait_start_time = current_time + 1;
        }
    }
};

class Elevator {
public:
    int idle_time; // 电梯空闲时间
    int run_time;  // 电梯运行时间
    int id;
    int current_floor;
    int target_floor;
    Direction direction;
    int capacity;
    int wait_time;
    std::vector<Passenger*> passengers;
    std::vector<Passenger*> require;
    std::vector<int> allowed_floors; // 新增，表示电梯可以到达的楼层

    Elevator(int id, int capacity) : id(id), current_floor(1), target_floor(-1), direction(IDLE), capacity(capacity), wait_time(0), idle_time(0), run_time(0) {
        // 根据 id 初始化 allowed_floors
        if (id == 0 || id == 1) {
            for (int i = 1; i <= FLOORS; ++i) allowed_floors.push_back(i);
        }
        else if (id == 2 || id == 3) {
            allowed_floors.push_back(1);
            for (int i = 25; i <= FLOORS; ++i) allowed_floors.push_back(i);
        }
        else if (id == 4 || id == 5) {
            for (int i = 1; i <= 25; ++i) allowed_floors.push_back(i);
        }
        else if (id == 6 || id == 7) {
            allowed_floors.push_back(1);
            for (int i = 2; i <= FLOORS; i += 2) allowed_floors.push_back(i);
        }
        else if (id == 8 || id == 9) {
            for (int i = 1; i < FLOORS; i += 2) allowed_floors.push_back(i);
        }
    }

    // 新增，判断电梯是否可以到达指定楼层
    bool can_reach(int floor) {
        return std::find(allowed_floors.begin(), allowed_floors.end(), floor) != allowed_floors.end();
    }

    bool is_full() {
        return passengers.size() >= capacity;
    }

    void add_passenger(Passenger* passenger) {
        if (!is_full()) {
            passengers.push_back(passenger);
            passenger->status = IN_ELEVATOR;
            passenger->current_floor = current_floor;
        }
    }

    void remove_passenger(Passenger* passenger) {
        passengers.erase(std::remove(passengers.begin(), passengers.end(), passenger), passengers.end());
        passenger->status = ARRIVED;
        passenger->current_floor = current_floor;
    }

    void add_require(Passenger* passenger) {
        require.push_back(passenger);
        target_floor = passenger->current_floor;
    }

    // 在电梯运行或空闲时更新相应的时间
    void update_time() {
        if (direction == IDLE) {
            idle_time++;
        }
        else {
            run_time++;
        }
    }

    // 获取当前电梯的请求总数
    int getRequestCount() {
        return require.size();
    }
};

int find_closest_elevator(std::vector<Elevator>& elevators, int floor, int floor2, Direction direction) {
    auto is_request_valid = [&elevators](Elevator& elevator, int passenger_current_floor, int passenger_target_floor, Direction passenger_direction) {
        if (elevator.direction == IDLE) {
            return true;
        }
        if (elevator.direction == UP) {
            if (passenger_current_floor >= elevator.current_floor && passenger_direction == UP && passenger_target_floor <= elevator.target_floor) {
                return true;
            }
        }
        else if (elevator.direction == DOWN) {
            if (passenger_current_floor <= elevator.current_floor && passenger_direction == DOWN && passenger_target_floor >= elevator.target_floor) {
                return true;
            }
        }
        return false;
    };

    int min_distance = INT_MAX;
    int closest_elevator = -1;

    for (Elevator& elevator : elevators) {
        // 判断电梯是否已满或者无法到达乘客的当前楼层或目标楼层
        if (elevator.is_full() || !elevator.can_reach(floor) || !elevator.can_reach(floor2)) {
            continue;
        }
        bool passenger_already_in_require = false;
        for (Elevator& other_elevator : elevators) {
            if (&elevator != &other_elevator) {
                for (Passenger* required_passenger : other_elevator.require) {
                    if (required_passenger->current_floor == floor && required_passenger->target_floor == floor2) {
                        passenger_already_in_require = true;
                        break;
                    }
                }
                if (passenger_already_in_require) {
                    break;
                }
            }
        }
        if (passenger_already_in_require) {
            continue;
        }

        int distance = abs(elevator.current_floor - floor);

        if (distance < min_distance) {
            bool is_direction_match = (elevator.direction == direction || elevator.direction == IDLE);
            bool is_request_in_path = (elevator.direction == UP && floor >= elevator.current_floor) ||
                (elevator.direction == DOWN && floor <= elevator.current_floor);

            bool valid_target_direction = true;

            if (is_request_valid(elevator, floor, floor2, direction) &&
                (elevator.direction == IDLE || (is_direction_match && is_request_in_path && valid_target_direction))) {
                min_distance = distance;
                closest_elevator = elevator.id;
            }
        }
    }

    return closest_elevator;
}

int main() {
    // 设置参数
    srand(time(NULL));
    int K = 15; // 每个电梯容量
    int N = PASSENGERS; // 总共乘客数量
    int M = 2; // 乘客在几分钟内到达
    int S = 1; // 电梯运行速度
    int T = 2; // 乘客上下速度

    // 生成电梯
    std::vector<Elevator> elevators;
    for (int i = 0; i < ELEVATORS; i++) {
        elevators.push_back(Elevator(i, K));
    }

    // 生成乘客
    std::vector<Passenger> passengers;
    for (int i = 0; i < N; i++) {
        int current_floor = rand() % FLOORS + 1;
        int target_floor;
        do {
            target_floor = rand() % FLOORS + 1;
        } while (target_floor == current_floor);

        int arrival_time;
        int random_percentage = rand() % 100 + 1;
        if (random_percentage <= 10) { // 10% 的乘客在0~1分钟到达
            arrival_time = rand() % (1 * 60);
        }
        else if (random_percentage <= 80) { // 70% 的乘客在1~2分钟到达
            arrival_time = (1 * 60) + rand() % (1 * 60);
        }
        else { // 20% 的乘客在2~3分钟到达
            arrival_time = (2 * 60) + rand() % (1 * 60);
        }

        int rides_left = rand() % 10 + 1;
        int wait_time = 9999999;
        passengers.push_back(Passenger(i, current_floor, target_floor, arrival_time, rides_left, wait_time));
    }

    int time = 0;
    while (true) {

        // 判断是否所有乘客的 rides_left 都不大于 0
        bool all_rides_finished = true;
        for (Passenger& passenger : passengers) {
            if (passenger.rides_left > 0) {
                all_rides_finished = false;
                break;
            }
        }

        if (all_rides_finished) {
            break;
        }

        time++;

        // 更新电梯
        for (Elevator& elevator : elevators) {
            elevator.update_time();
            if (elevator.wait_time > 0) {
                elevator.wait_time--;
                continue;
            }

            if (elevator.passengers.empty() && elevator.require.empty()) {
                elevator.direction = IDLE;
            }

            if (elevator.direction == IDLE) {
                if (!elevator.require.empty()) {
                    elevator.target_floor = elevator.require[0]->current_floor;
                    elevator.direction = (elevator.target_floor > elevator.current_floor) ? UP : DOWN;
                }
                if (!elevator.passengers.empty() && elevator.require.empty()) {
                    elevator.target_floor = elevator.passengers[0]->target_floor;
                    elevator.direction = (elevator.target_floor > elevator.current_floor) ? UP : DOWN;
                }
            }

            if (elevator.direction != IDLE) {
                elevator.current_floor += (elevator.direction == UP) ? 1 : -1;
                elevator.wait_time = S - 1; // 设置等待时间为移动所需的时间
                if (elevator.current_floor >= FLOORS) {
                    elevator.current_floor = FLOORS;
                    elevator.direction = DOWN;
                }
                if (elevator.current_floor <= 1) {
                    elevator.current_floor = 1;
                    elevator.direction = UP;
                }
            }

            // 当电梯到达目标楼层时，将电梯的目标楼层设置为乘客的目标楼层
            if (elevator.current_floor == elevator.target_floor) {
                if (!elevator.passengers.empty()) {
                    elevator.target_floor = elevator.passengers[0]->target_floor;
                    elevator.direction = (elevator.target_floor > elevator.current_floor) ? UP : DOWN;
                }
            }

            // 乘客上电梯时设置等待时间
            for (auto it = elevator.require.begin(); it != elevator.require.end();) {
                Passenger* passenger = *it;
                if (passenger->current_floor == elevator.current_floor && passenger->status == WAITING && passenger->rides_left > 0) {
                    elevator.add_passenger(passenger);
                    it = elevator.require.erase(it);
                    elevator.wait_time += T - 1; // 设置等待时间为上下乘客所需的时间
                }
                else {
                    ++it;
                }
            }

            // 乘客下电梯时设置等待时间
            auto it = elevator.passengers.begin();
            while (it != elevator.passengers.end()) {
                Passenger* passenger = *it;
                if (passenger->target_floor == elevator.current_floor) {
                    if (passenger->rides_left > 1) {
                        passenger->rides_left--;
                        passenger->current_floor = passenger->target_floor; // 更新乘客的current_floor
                        passenger->status = ARRIVED; // 更改乘客状态为到达
                        passenger->wait_time = rand() % 111 + 10;
                        do {
                            passenger->target_floor = rand() % FLOORS + 1;
                        } while (passenger->target_floor == elevator.current_floor);
                    }
                    else {
                        passenger->rides_left--;
                        passenger->status = ARRIVED;
                        passenger->current_floor = elevator.current_floor;
                    }
                    it = elevator.passengers.erase(it);
                    elevator.wait_time += T - 1; // 设置等待时间为上下乘客所需的时间
                }
                else {
                    ++it;
                }
            }
        }

        // 更新乘客
        for (Passenger& passenger : passengers) {
            if (passenger.status == WAITING && passenger.arrival_time <= time && passenger.rides_left > 0) {
                if (!passenger.has_requested_elevator) {
                    passenger.start_waiting(time);
                }
            }
            if (passenger.status == IN_ELEVATOR) {
                passenger.end_waiting(time);
            }
            if (passenger.status == ARRIVED && passenger.wait_time > 0) {
                passenger.wait_time--;
            }
            else if (passenger.status == ARRIVED && passenger.wait_time == 0) {
                passenger.status = WAITING;
                passenger.has_requested_elevator = false;
            }
            if (passenger.status == WAITING && passenger.arrival_time <= time && passenger.rides_left > 0) {
                Direction passenger_direction = (passenger.target_floor > passenger.current_floor) ? UP : DOWN;
                int closest_elevator_id = find_closest_elevator(elevators, passenger.current_floor, passenger.target_floor, passenger_direction);

                if (closest_elevator_id != -1) {
                    Elevator& closest_elevator = elevators[closest_elevator_id];

                    if (!closest_elevator.is_full()) {
                        if (closest_elevator.current_floor == passenger.current_floor) {
                            closest_elevator.add_passenger(&passenger);
                            closest_elevator.require.erase(std::remove(closest_elevator.require.begin(), closest_elevator.require.end(), &passenger), closest_elevator.require.end());
                        }
                        else {
                            if (!passenger.has_requested_elevator) {
                                closest_elevator.add_require(&passenger);
                                passenger.has_requested_elevator = true;
                            }
                        }
                    }
                }
            }
        }

        // 界面
        std::cout << "Time: " << time << std::endl;

        // 计算当前所有电梯的请求总数
        int total_requests = 0;
        for (int i = 0; i < elevators.size(); i++) {
            total_requests += elevators[i].getRequestCount();
        }

        // 如果请求总数超过总乘客数的15%，则输出消息
        if (total_requests > N * 0.15) {
            std::cout << "当前为高峰期！" << std::endl;
        }

        for (Elevator& elevator : elevators) {
            std::cout << "Elevator " << elevator.id << ": ";
            std::cout << "Floor " << elevator.current_floor << ", ";
            std::cout << (elevator.direction == UP ? "Up" : elevator.direction == DOWN ? "Down" : "Idle") << ", ";
            std::cout << "Passengers: ";

            for (Passenger* passenger : elevator.passengers) {
                std::cout << "乘客" << passenger->id << ":" << passenger->current_floor << "-" << passenger->target_floor << " ";
            }

            std::cout << std::endl;
            std::cout << "Require: ";

            for (Passenger* passenger : elevator.require) {
                std::cout << "乘客" << passenger->id << ":" << passenger->current_floor << "-" << passenger->target_floor << " ";
            }
            std::cout << std::endl;
        }

        std::cout << std::endl;

        // 逐步运行
        /*
        int userInput;
        std::cout << "Enter 1 to continue: ";
        std::cin >> userInput;
        while (userInput != 1) {
            std::cout << "Invalid input. Enter 1 to continue: ";
            std::cin >> userInput;
        }*/
    }
    // 仿真结束后输出电梯的运行时间和空闲时间
    for (Elevator& elevator : elevators) {
        std::cout << "E" << elevator.id << " 空闲了 " << elevator.idle_time << " 秒。" << std::endl;
        std::cout << "E" << elevator.id << " 运行了 " << elevator.run_time << " 秒。" << std::endl;
    }
    // 仿真结束后输出每个乘客的等待时间
    for (Passenger& passenger : passengers) {
        std::cout << "乘客 " << passenger.id << " 总共等待了 " << passenger.total_wait_time << " 秒。" << std::endl;
    }
    return 0;
}
