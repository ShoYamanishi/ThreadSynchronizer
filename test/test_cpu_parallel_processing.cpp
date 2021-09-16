#include <iostream>
#include <iomanip>
#include <thread>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "thread_synchronizer.h"

using namespace std;


class TestCaseWithTimeMeasurements {

  protected:
    string         m_type_string;
    vector<double> m_measured_times;
    double         m_mean_times;
    double         m_stddev_times;

  public:

    TestCaseWithTimeMeasurements( const string type )
        :m_type_string        ( type  )
        ,m_mean_times         ( 0.0   )
        ,m_stddev_times       ( 0.0   )
    {;}


    virtual ~TestCaseWithTimeMeasurements(){;}

    void addTime( const double microseconds ) {

        m_measured_times.push_back( microseconds );
    }

    const string testType() { return m_type_string; }

    virtual const string testCaseSpecificOutput() { return ""; }

    void calculateMeanStddevOfTime() {

        m_mean_times = 0.0;

        const double len = m_measured_times.size();

        for ( auto v : m_measured_times ) {
            m_mean_times += v;
        }

        m_mean_times /= len;

        m_stddev_times = 0.0;

        for ( auto v : m_measured_times ) {

            const double diff = v - m_mean_times;
            const double sq   = diff * diff;
            m_stddev_times += sq;
        }
        m_stddev_times /= ( len - 1 );
    }

    virtual void print( const string preamble ) {
        cout << setprecision(4);
        cout << "RESULT";
        cout << "\t";
        cout << preamble;
        cout << "\t";
        cout << m_type_string;
        cout << "\t";
        cout << "mean: " << (m_mean_times * 1000.0) << " [ms]" ;
        cout << "\t";
        cout << "stddev: " << (m_stddev_times * 1000.0) << " [ms]" ;
        cout << "\t";
        cout << testCaseSpecificOutput();
        cout << "\n";
    }

    virtual void run() = 0;
};

class TestExecutor {

  protected:
    vector< shared_ptr< TestCaseWithTimeMeasurements > > m_test_cases;

    const int m_num_trials;

  public:
    TestExecutor( const int num_trials )
        :m_num_trials( num_trials ) {;}

    virtual ~TestExecutor(){;}

    void addTestCase( shared_ptr< TestCaseWithTimeMeasurements>&& c ) {
        m_test_cases.emplace_back( c );
    }

    virtual void   prepareForBatchRuns   ( const int test_case ){;}
    virtual void   cleanupAfterBatchRuns ( const int test_case ){;}
    virtual void   prepareForRun         ( const int test_case, const int num ){;}
    virtual void   cleanupAfterRun       ( const int test_case, const int num ){;}

    virtual const string preamble () { return ""; }

    void execute() {

        for ( int i = 0; i < m_test_cases.size(); i++ ) {

            auto test_case = m_test_cases[i];

            cout << "Testing [" << test_case->testType() << "] ";

            prepareForBatchRuns(i);

            for ( int j = 0; j < m_num_trials + 1; j++ ) {

                cout << "." << flush;

                prepareForRun(i, j);

                auto time_begin = chrono::high_resolution_clock::now();        

                test_case->run();

                auto time_end = chrono::high_resolution_clock::now();        

                cleanupAfterRun(i, j);

                chrono::duration<double> time_diff = time_end - time_begin;

                if (j > 0) {
                    // discard the first run.
                    test_case->addTime( time_diff.count() );
                }
            }
            cout << "\n";

            cleanupAfterBatchRuns(i);
           
        }

        for ( int i = 0; i < m_test_cases.size(); i++ ) {

            auto t = m_test_cases[i];

            t->calculateMeanStddevOfTime();

            t->print( preamble() );
        }
    }
};


class CyclicScheduler : public TestCaseWithTimeMeasurements {

    const int                   m_num_oscillations;
    const int                   m_num_threads;

    WaitNotifySingle            m_wait_notify_master;                
    vector< WaitNotifySingle* > m_wait_notify_workers;
    vector< thread >            m_threads;

    atomic_int                  m_counter;

  public:

    CyclicScheduler( const int num_threads, const int num_oscillations )
        :TestCaseWithTimeMeasurements("cyclic scheduler ")
        ,m_num_oscillations( num_oscillations )
        ,m_num_threads     ( num_threads )
        ,m_counter         ( 0 )

    {
        m_type_string += "[";
        m_type_string += std::to_string(num_threads);
        m_type_string += ", ";
        m_type_string += std::to_string(num_oscillations);
        m_type_string += " * ";
        m_type_string += std::to_string(num_oscillations);
        m_type_string += "]";

        for ( int i = 0; i < num_threads; i++ ) {
            m_wait_notify_workers.emplace_back( new WaitNotifySingle );
        }

        auto task = [&]( const int num ) {

            while ( true ) {

                m_wait_notify_workers[ num ]->wait();

                if ( m_wait_notify_workers[ num ]->isTerminating() ) {
                    break;
                }
                // do task
                if ( num == 0 ) {
                    m_counter.fetch_add( 1, memory_order::acq_rel );
                }
                if ( num == m_num_threads - 1 ) {

                    if ( m_counter.load( memory_order::acquire ) == m_num_oscillations ) {
                        m_wait_notify_master.notify();
                        continue;
                    }
                }

                m_wait_notify_workers[ (num + 1)% m_num_threads ]->notify();

                if ( m_wait_notify_workers[ num ]->isTerminating() ) {
                    break;
                }
            }
        };

        for ( int i = 0; i < num_threads; i++ ) {

            m_threads.emplace_back( task, i );    

        }

    }

    void run() {

        for ( int i = 0; i < m_num_oscillations; i++ ) {

            m_counter.store( 0,  memory_order::release );

            m_wait_notify_workers[0]->notify();

            m_wait_notify_master.wait();

        }

    }

    virtual ~CyclicScheduler() {

        for ( int i = 0; i < m_num_threads; i++ ) {
            m_wait_notify_workers[i]->terminate();
        }

        for ( auto& t : m_threads ) {
            t.join();
        }

        for ( int i = 0; i < m_num_threads; i++ ) {
            delete m_wait_notify_workers[i];
        }
    }

};


class ParallelSchedulerWithPooling : public TestCaseWithTimeMeasurements {

    const int                   m_num_oscillations;
    const int                   m_num_threads;

    WaitNotifyMultipleWaiters   m_wait_notify_fan_out;
    WaitNotifyMultipleNotifiers m_wait_notify_fan_in;

    vector< thread >            m_threads;

  public:

    ParallelSchedulerWithPooling( const int num_threads, const int num_oscillations )
        :TestCaseWithTimeMeasurements("parallel scheduler ")
        ,m_num_oscillations   ( num_oscillations )
        ,m_num_threads        ( num_threads )
        ,m_wait_notify_fan_out( num_threads )
        ,m_wait_notify_fan_in ( num_threads )
    {
        m_type_string += "[";
        m_type_string += std::to_string(m_num_threads);
        m_type_string += ", ";
        m_type_string += std::to_string(m_num_oscillations);
        m_type_string += "]";

        auto task = [&]( const int num ) {

            while ( true ) {

                m_wait_notify_fan_out.wait( num );
                if ( m_wait_notify_fan_out.isTerminating() ) {
                    break;
                }
                // do task 1

                m_wait_notify_fan_in.notify();
                if ( m_wait_notify_fan_in.isTerminating() ) {
                    break;
                }
            }
        };

        for ( int i = 0; i < m_num_threads; i++ ) {
            m_threads.emplace_back( task, i );    
        }
    }

    virtual void run()
    {
        for ( int i = 0; i < m_num_oscillations; i++ ) {

            m_wait_notify_fan_out.notify();

            m_wait_notify_fan_in.wait();
        }
    }

    ~ParallelSchedulerWithPooling()
    {
        m_wait_notify_fan_out.terminate();
        m_wait_notify_fan_in.terminate();

        for ( auto& t : m_threads ) {
            t.join();
        }
    }
};


class ParallelSchedulerWithPoolingWithMidSyncOld : public TestCaseWithTimeMeasurements {

    const int                   m_num_oscillations;
    const int                   m_num_threads;

    WaitNotifyMultipleWaiters   m_wait_notify_fan_out;
    WaitNotifyMultipleNotifiers m_wait_notify_fan_in;
    WaitNotifyNxN               m_wait_notify_sync;

    vector< thread >            m_threads;

  public:

    ParallelSchedulerWithPoolingWithMidSyncOld( const int num_threads, const int num_oscillations )
        :TestCaseWithTimeMeasurements("parallel scheduler with mid-sync")
        ,m_num_oscillations   ( num_oscillations )
        ,m_num_threads        ( num_threads )
        ,m_wait_notify_fan_out( num_threads )
        ,m_wait_notify_fan_in ( num_threads )
        ,m_wait_notify_sync   ( num_threads )
    {
        m_type_string += "[";
        m_type_string += std::to_string(m_num_threads);
        m_type_string += ", ";
        m_type_string += std::to_string(m_num_oscillations);
        m_type_string += "]";

        auto task1 = [&]( const int num ) {

            while ( true ) {
                m_wait_notify_fan_out.wait( num );
                if ( m_wait_notify_fan_out.isTerminating() ) {
                    break;
                }

                // do task 1

                m_wait_notify_sync.notify();
                if ( m_wait_notify_sync.isTerminating() ) {
                    break;
                }
            }
        };

        auto task2 = [&]( const int num ) {

            while ( true ) {

                m_wait_notify_sync.wait( num );
                if ( m_wait_notify_sync.isTerminating() ) {
                    break;
                }

                // do task 2

                m_wait_notify_fan_in.notify();
                if ( m_wait_notify_fan_in.isTerminating() ) {
                    break;
                }
            }
        };

        for ( int i = 0; i < m_num_threads; i++ ) {
            m_threads.emplace_back( task1, i );
            m_threads.emplace_back( task2, i );
        }
    }

    virtual void run()
    {
        for ( int i = 0; i < m_num_oscillations; i++ ) {
            m_wait_notify_fan_out.notify();
            m_wait_notify_fan_in.wait();
        }
    }

    ~ParallelSchedulerWithPoolingWithMidSyncOld()
    {
        m_wait_notify_fan_out.terminate();
        m_wait_notify_sync.terminate();
        m_wait_notify_fan_in.terminate();

        for ( auto& t : m_threads ) {
            t.join();
        }
    }
};


class ParallelSchedulerWithPoolingWithMidSync : public TestCaseWithTimeMeasurements {

    const int                   m_num_oscillations;
    const int                   m_num_threads;

    WaitNotifyMultipleWaiters   m_wait_notify_fan_out;
    WaitNotifyMultipleNotifiers m_wait_notify_fan_in;
    WaitNotifyEachOther         m_wait_notify_sync;

    vector< thread >            m_threads;

  public:

    ParallelSchedulerWithPoolingWithMidSync( const int num_threads, const int num_oscillations )
        :TestCaseWithTimeMeasurements("parallel scheduler with mid-sync")
        ,m_num_oscillations   ( num_oscillations )
        ,m_num_threads        ( num_threads )
        ,m_wait_notify_fan_out( num_threads )
        ,m_wait_notify_fan_in ( num_threads )
        ,m_wait_notify_sync   ( num_threads )
    {
        m_type_string += "[";
        m_type_string += std::to_string(m_num_threads);
        m_type_string += ", ";
        m_type_string += std::to_string(m_num_oscillations);
        m_type_string += "]";

        auto task = [&]( const int num ) {

            while ( true ) {
                m_wait_notify_fan_out.wait( num );
                if ( m_wait_notify_fan_out.isTerminating() ) {
                    break;
                }

                // do task1

                // wait until all the threads reach here.
                m_wait_notify_sync.syncThreads( num );
                if ( m_wait_notify_sync.isTerminating() ) {
                    break;
                }

                // do task2

                m_wait_notify_fan_in.notify();
                if ( m_wait_notify_fan_in.isTerminating() ) {
                    break;
                }
            }
        };

        for ( int i = 0; i < m_num_threads; i++ ) {
            m_threads.emplace_back( task, i );
        }
    }

    virtual void run()
    {
        for ( int i = 0; i < m_num_oscillations; i++ ) {
            m_wait_notify_fan_out.notify();
            m_wait_notify_fan_in.wait();
        }
    }

    ~ParallelSchedulerWithPoolingWithMidSync()
    {
        m_wait_notify_fan_out.terminate();
        m_wait_notify_sync.terminate();
        m_wait_notify_fan_in.terminate();

        for ( auto& t : m_threads ) {
            t.join();
        }
    }
};

        
class ParallelSchedulerNaive : public TestCaseWithTimeMeasurements {

    const int m_num_threads;
    const int m_num_iteration;

  public:

    ParallelSchedulerNaive( const int num_threads, const int num_iteration )
        :TestCaseWithTimeMeasurements( "parallel scheduler naive" )
        ,m_num_threads   ( num_threads   )
        ,m_num_iteration ( num_iteration )
    {
        m_type_string += "[";
        m_type_string += std::to_string(num_threads);
        m_type_string += ",";
        m_type_string += std::to_string(m_num_iteration);
        m_type_string += "]";
    }

    virtual void run() {

        auto task = [&]( const size_t thread_index ) {;}; // do nothing

        for ( int i = 0; i < m_num_iteration; i++ ) {        

            std::vector<std::thread> threads;

            for ( size_t i = 0; i < m_num_threads; i++ ) {
                threads.emplace_back( task, i );
            }

            for ( auto& t : threads ) {
                t.join();
            }
        }
    }

    virtual ~ParallelSchedulerNaive() {;}
};


static const size_t NUM_TRIALS       = 10;
static const size_t NUM_OSCILLATIONS = 100;
static const size_t NUM_ITERATIONS_PARALLEL = 10000;

int main( int argc, char* argv[] ) {

    TestExecutor e( NUM_TRIALS );

    e.addTestCase( make_shared< CyclicScheduler >             (   2, NUM_OSCILLATIONS ) );
    e.addTestCase( make_shared< CyclicScheduler >             (   3, NUM_OSCILLATIONS ) );
    e.addTestCase( make_shared< CyclicScheduler >             (   5, NUM_OSCILLATIONS ) );
    e.addTestCase( make_shared< CyclicScheduler >             (  10, NUM_OSCILLATIONS ) );
    e.addTestCase( make_shared< CyclicScheduler >             ( 100, NUM_OSCILLATIONS ) );

    e.addTestCase( make_shared< ParallelSchedulerNaive >      (   4, NUM_ITERATIONS_PARALLEL ) );
    e.addTestCase( make_shared< ParallelSchedulerNaive >      (  16, NUM_ITERATIONS_PARALLEL ) );
    e.addTestCase( make_shared< ParallelSchedulerNaive >      (  64, NUM_ITERATIONS_PARALLEL ) );

    e.addTestCase( make_shared< ParallelSchedulerWithPooling >(   4, NUM_ITERATIONS_PARALLEL ) );
    e.addTestCase( make_shared< ParallelSchedulerWithPooling >(  16, NUM_ITERATIONS_PARALLEL ) );
    e.addTestCase( make_shared< ParallelSchedulerWithPooling >(  64, NUM_ITERATIONS_PARALLEL ) );

    e.addTestCase( make_shared< ParallelSchedulerWithPoolingWithMidSync >(    4, NUM_ITERATIONS_PARALLEL ) );
    e.addTestCase( make_shared< ParallelSchedulerWithPoolingWithMidSync >(   16, NUM_ITERATIONS_PARALLEL ) );
    e.addTestCase( make_shared< ParallelSchedulerWithPoolingWithMidSync >(   64, NUM_ITERATIONS_PARALLEL ) );

    e.execute();

    return 0;
}

