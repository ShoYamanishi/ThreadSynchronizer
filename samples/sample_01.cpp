#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

int main( int argc, char* argv[] ) {

    mutex  mx;

    auto task1 = [&] {

        unique_lock<mutex> lk( mx, defer_lock );

        while (true) {

            lk.lock();
            cout << "task 1\n" << flush;
            lk.unlock();
        }
    };

    auto task2 = [&] {

        unique_lock<mutex> lk( mx, defer_lock );

        while (true) {

            lk.lock();
            cout << "task 2\n" << flush;
            lk.unlock();
        }
    };

    thread th1( task1 );
    thread th2( task2 );

    th1.join();
    th2.join();

    return 0;
}
