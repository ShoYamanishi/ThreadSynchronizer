#include <iostream>
#include <thread>
#include <atomic>
#include "thread_synchronizer.h"

using namespace std;

int main( int argc, char* argv[] ) {

    WaitNotifyMultipleWaiters   wn_fan_out(3);
    WaitNotifyMultipleNotifiers wn_fan_in(3);

    mutex      mt;
    atomic_int cnt(0);

    auto task1 = [&] {
        while (true) {

            wn_fan_out.wait(0);
            if (wn_fan_out.isTerminating())
                break;
            mt.lock();
            cout << "task 1 cnt:" << to_string(cnt.load()) << "\n" << flush;
            mt.unlock();

            wn_fan_in.notify();
        }
    };

    auto task2 = [&] {
        while (true) {

            wn_fan_out.wait(1);
            if (wn_fan_out.isTerminating())
                break;
            mt.lock();
            cout << "task 2 cnt:" << to_string(cnt.load()) << "\n" << flush;
            mt.unlock();

            wn_fan_in.notify();
        }
    };

    auto task3 = [&] {
        while (true) {

            wn_fan_out.wait(2);
            if (wn_fan_out.isTerminating())
                break;
            mt.lock();
            cout << "task 3 cnt:" << to_string(cnt.load()) << "\n" << flush;
            mt.unlock();

            wn_fan_in.notify();
        }
    };

    thread th1( task1 );
    thread th2( task2 );
    thread th3( task3 );

    for ( int i = 0; i < 10 ; i++ ) {

        wn_fan_out.notify();
        wn_fan_in.wait();
        cnt++;
    }
    wn_fan_out.terminate();

    th1.join();
    th2.join();
    th3.join();

    return 0;
}
