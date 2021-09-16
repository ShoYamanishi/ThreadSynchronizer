#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

int main( int argc, char* argv[] ) {

    mutex              mx1, mx2;
    condition_variable cv1, cv2;

    auto task1 = [&] {

        unique_lock<mutex> lk1( mx1, defer_lock );
        unique_lock<mutex> lk2( mx2, defer_lock );

        while (true) {

            lk1.lock();
            cv1.wait( lk1 );
            lk1.unlock();

            cout << "task 1\n" << flush;

            cv2.notify_one();
        }
    };

    // task2 is a mirror image of task1.
    auto task2 = [&] {

        unique_lock<mutex> lk2( mx2, defer_lock );
        unique_lock<mutex> lk1( mx1, defer_lock );

        while (true) {

            lk2.lock();
            cv2.wait( lk2 );
            lk2.unlock();

            cout << "task 2\n" << flush;

            cv1.notify_one();
        }
    };

    thread th1( task1 );
    thread th2( task2 );

    th1.join();
    th2.join();

    return 0;
}
