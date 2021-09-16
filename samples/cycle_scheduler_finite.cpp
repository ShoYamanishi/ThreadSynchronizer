#include <iostream>
#include <thread>
#include <atomic>
#include "thread_synchronizer.h"

using namespace std;

int main( int argc, char* argv[] ) {

    WaitNotifySingle wn_parent, wn1, wn2, wn3, wn4, wn5;

    atomic_int counter(0);

    auto task1 = [&] {
        while (true) {
            wn1.wait();
            if ( wn1.isTerminating() )
                break;
            cout << "task 1\tcnt: " << counter << "\n" << flush;
            counter++;
            wn2.notify();
        }
    };

    auto task2 = [&] {
        while (true) {
            wn2.wait();
            if ( wn2.isTerminating() )
                break;
            cout << "task 2\tcnt: " << counter << "\n" << flush;
            wn3.notify();
        }
    };

    auto task3 = [&] {
        while (true) {
            wn3.wait();
            if ( wn3.isTerminating() )
                break;
            cout << "task 3\tcnt: " << counter << "\n" << flush;
            wn4.notify();
        }
    };

    auto task4 = [&] {
        while (true) {
            wn4.wait();
            if ( wn4.isTerminating() )
                break;
            cout << "task 4\tcnt: " << counter << "\n" << flush;
            wn5.notify();
        }
    };

    auto task5 = [&] {
        while (true) {
            wn5.wait();
            if ( wn5.isTerminating() )
                break;
            cout << "task 5\tcnt: " << counter << "\n" << flush;
            if (counter < 10) {
                wn1.notify();
            }
            else {            
                // finish
                wn_parent.notify();
            }
        }
    };

    thread th1( task1 );
    thread th2( task2 );
    thread th3( task3 );
    thread th4( task4 );
    thread th5( task5 );

    // prime task1 to start the oscillation.
    wn1.notify();

    // waiting for task5 to notify.
    wn_parent.wait();

    wn1.terminate();
    wn2.terminate();
    wn3.terminate();
    wn4.terminate();
    wn5.terminate();

    th1.join();
    th2.join();
    th3.join();
    th4.join();
    th5.join();

    return 0;
}
