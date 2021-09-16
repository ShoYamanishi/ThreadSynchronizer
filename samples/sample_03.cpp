#include <iostream>
#include <iomanip>
#include <thread>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace std;

int main( int argc, char* argv[] ) {

    mutex              mx1, mx2;
    condition_variable cv1, cv2;
    atomic_bool        cf1, cf2; // this flag is used together with notify() to avoid spurious wakeups.
    atomic_bool        wf1, wf2; // this flag is used to make sure the other task is waiting before calling notify()

    auto task1 = [&] {

        unique_lock<mutex> lk1( mx1, defer_lock );
        unique_lock<mutex> lk2( mx2, defer_lock );

        while (true) {

            lk1.lock();
            wf1.store( true, memory_order_release );
            cv1.wait( lk1, [&] { return cf1.load( memory_order_acquire ); } ); // --- (C)
            cf1.store( false, memory_order_release );
            lk1.unlock();

            cout << "task 1\n" << flush;

            while ( !wf2.load( memory_order_acquire ) ){;} // wait until task2 sets true to w2.
            wf2.store( false, memory_order_release ); // immediagely reset w2, as we know task2 has passed (A) below.
            lk2.lock();   // this lock()/unlock() pair makes sure that task2 is in condition wait at (B).
            lk2.unlock(); // if lock() is successful, we know task2 is in wait().
            cf2.store( true, memory_order_release );
            cv2.notify_one();
        }
    };

    // task2 is a mirror image of task1.
    auto task2 = [&] {

        unique_lock<mutex> lk2( mx2, defer_lock );
        unique_lock<mutex> lk1( mx1, defer_lock );

        while (true) {

            lk2.lock();
            wf2.store( true, memory_order_release ); // --- (A)
            cv2.wait( lk2, [&] { return cf2.load( memory_order_acquire ); } ); // --- (B)
            cf2.store( false, memory_order_release );
            lk2.unlock();

            cout << "task 2\n" << flush;

            while ( !wf1.load( memory_order_acquire ) ){;}
            wf1.store( false, memory_order_release );
            lk1.lock();
            lk1.unlock();
            cf1.store( true, memory_order_release );
            cv1.notify_one();
        }
    };

    thread th1( task1 );
    thread th2( task2 );

    // stimulate task1 to start the oscillation.
    // NOTE: there is a chance that the following notify_one() will not be caught by task1.
    //       if task1 is too slow to execute, and has not reached the point (C) above.
    //       the handling of such cases is omitted here for clarity.
    cf1.store( true, memory_order_release );
    cv1.notify_one();

    th1.join();
    th2.join();

    return 0;
}
