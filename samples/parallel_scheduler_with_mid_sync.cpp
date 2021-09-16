#include <iostream>
#include <thread>
#include <atomic>
#include "thread_synchronizer.h"

using namespace std;

static inline bool task_with_inner_loop( 
    const int            tid,
    WaitNotifyEachOther& s,
    const int            cnt,
    mutex&               lock_for_cout
) {
    for ( int i = 0; i < 4; i++ ) {

        lock_for_cout.lock();
        cout << "task: " << tid << " part 1 inner loop: " << i << " outer loop: " << cnt << "\n" << flush;
        lock_for_cout.unlock();

         s.syncThreads(tid); // --- (A)
         if (s.isTerminating())
            return true;

        lock_for_cout.lock();
        cout << "task: " << tid << " part 2 inner loop: " << i << " outer loop: " << cnt << "\n" << flush;
        lock_for_cout.unlock();

         s.syncThreads(tid); // --- (B)
         if (s.isTerminating())
            return true;
    }
    return false;
}


int main( int argc, char* argv[] ) {

    WaitNotifyMultipleWaiters   wn_fan_out(3);
    WaitNotifyMultipleNotifiers wn_fan_in(3);
    WaitNotifyEachOther         wn_mid_sync(3);

    mutex      mt;
    atomic_int cnt(0);


    auto task1 = [&] {
        while (true) {

            wn_fan_out.wait(0);
            if (wn_fan_out.isTerminating())
                break;

            auto exiting = task_with_inner_loop( 0, wn_mid_sync, cnt.load(), mt );
            if (exiting)
               break;

            wn_fan_in.notify();
        }
    };

    auto task2 = [&] {
        while (true) {

            wn_fan_out.wait(1);
            if (wn_fan_out.isTerminating())
                break;

            auto exiting = task_with_inner_loop( 1, wn_mid_sync, cnt.load(), mt );
            if (exiting)
               break;

            wn_fan_in.notify();
        }
    };

    auto task3 = [&] {
        while (true) {

            wn_fan_out.wait(2);
            if (wn_fan_out.isTerminating())
                break;

            auto exiting = task_with_inner_loop( 2, wn_mid_sync, cnt.load(), mt );
            if (exiting)
               break;

            wn_fan_in.notify();
        }
    };

    thread th1( task1 );
    thread th2( task2 );
    thread th3( task3 );

    for ( int i = 0; i < 10; i++ ) {

        wn_fan_out.notify();
        wn_fan_in.wait();
        cnt++;
    }

    wn_fan_out.terminate();
    wn_mid_sync.terminate();

    th1.join();
    th2.join();
    th3.join();

    return 0;
}
