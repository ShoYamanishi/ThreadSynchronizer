#include <iostream>
#include <thread>
#include "thread_synchronizer.h"

using namespace std;

int main( int argc, char* argv[] ) {

    WaitNotifySingle wn1, wn2;

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
            wn1.notify();
        }
    };

    thread th1( task1 );
    thread th2( task2 );

    // prime task1 to start the oscillation.
    wn1.notify();

    th1.join();
    th2.join();

    return 0;
}
