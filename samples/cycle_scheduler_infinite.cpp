#include <iostream>
#include <thread>
#include "thread_synchronizer.h"

using namespace std;

int main( int argc, char* argv[] ) {

    WaitNotifySingle wn1, wn2, wn3, wn4, wn5;

    auto task1 = [&] {
        while (true) {
            wn1.wait();
            cout << "task 1\n" << flush;
            wn2.notify();
        }
    };

    auto task2 = [&] {
        while (true) {
            wn2.wait();
            cout << "task 2\n" << flush;
            wn3.notify();
        }
    };

    auto task3 = [&] {
        while (true) {
            wn3.wait();
            cout << "task 3\n" << flush;
            wn4.notify();
        }
    };

    auto task4 = [&] {
        while (true) {
            wn4.wait();
            cout << "task 4\n" << flush;
            wn5.notify();
        }
    };

    auto task5 = [&] {
        while (true) {
            wn5.wait();
            cout << "task 5\n" << flush;
            wn1.notify();
        }
    };

    thread th1( task1 );
    thread th2( task2 );
    thread th3( task3 );
    thread th4( task4 );
    thread th5( task5 );

    // prime task1 to start the oscillation.
    wn1.notify();

    th1.join();
    th2.join();
    th3.join();
    th4.join();
    th5.join();

    return 0;
}
