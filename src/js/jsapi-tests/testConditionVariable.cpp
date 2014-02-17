/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "threading/ConditionVariable.h"
#include "threading/Mutex.h"
#include "threading/Thread.h"
#include "jsapi-tests/tests.h"

#include <stdint.h>


class ConditionVariableTest: public JSAPITest {
    static const int NUM_TESTS = 8;
    static const int NUM_THREADS = 8;
    static const int NUM_ROUNDS = 250;

  public:
    ConditionVariableTest();
    ~ConditionVariableTest();

    virtual const char* name();
    virtual bool run(JS::HandleObject global);

  private:
    static void testThreadEntry(void* arg);
    bool testThread();
    bool rendezvous();

    js::Mutex rendezvousLock;
    js::ConditionVariable rendezvousCondVar;

    js::Mutex testLock;
    js::ConditionVariable testCondVar;

    bool failed;
};


ConditionVariableTest::ConditionVariableTest()
  : rendezvousLock(),
    rendezvousCondVar(),
    testLock(),
    testCondVar(),
    failed(false)
{ }


ConditionVariableTest::~ConditionVariableTest() { 
}


const char* ConditionVariableTest::name() {
    return "testConditionVariable";
}


bool ConditionVariableTest::run(JS::HandleObject global) {
    int i;
    bool r;
    js::Thread threads[NUM_THREADS];

    r = rendezvousLock.initialize();
    CHECK(r);
    r = rendezvousCondVar.initialize();
    CHECK(r);
    testLock.initialize();
    CHECK(r);
    testCondVar.initialize();
    CHECK(r);
    
    for (i = 0; i < NUM_THREADS; i++) {
        js::Thread& thread = threads[i];
        r = thread.start(testThreadEntry, this);
        CHECK(r);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        js::Thread& thread = threads[i];
        thread.join();
    }

    return !failed;
}


void ConditionVariableTest::testThreadEntry(void* arg) {
    ConditionVariableTest& context = *reinterpret_cast<ConditionVariableTest*>(arg);
    
    if (!context.testThread())
        context.failed = true;
}


bool ConditionVariableTest::testThread() {
    static volatile int testCounter1 = 0;
    static volatile int testCounter2 = 0;

    int currentTest = 0;
    int currentRound = 0;

    while (currentRound < NUM_ROUNDS) {
        rendezvous();

        // Yes, these counters are reset by every test thread. You think that's
        // overkill? Right on.
        testCounter1 = 0;
        testCounter2 = 0;

        rendezvous();
        
        switch (currentTest) {
          case 0:
            // Dummy test - rendezvous only.
            break;

          case 1: {
            // * first N-1 threads sleep 
            // * Nth thread broadcasts
            // * this is essentially the same as the rendezvous
            js::AutoMutexLock lock(testLock);

            if (++testCounter1 < NUM_THREADS)
                testCondVar.wait(testLock);
            else
                testCondVar.broadcast();

            CHECK(testCounter1 == NUM_THREADS);

            break;
          }

          case 2: {
            // * first N-1 threads sleep then signal
            // * Nth thread signals then sleeps then signals again
            // * the last in this test should be lost
            js::AutoMutexLock lock(testLock);

            if (++testCounter1 < NUM_THREADS) {
                testCondVar.wait(testLock);
                testCondVar.signal();
            } else {
                testCondVar.signal();
                testCondVar.wait(testLock);
                testCondVar.signal();
            }

            CHECK(testCounter1 == NUM_THREADS);

            break;
          }

          case 3: {
            // * all threads sleep and timeout
            js::AutoMutexLock lock(testLock);
            
            bool woken = testCondVar.wait(testLock, 1);
            CHECK(!woken);
            
            break;
          }

          case 4: {
            // * all threads sleep with a timeout
            // * the first thread is woken up by a timeout
            // * other threads either time out or are woken up by a signal
            // * all threads signal after waking up
            testLock.lock();

            bool woken = testCondVar.wait(testLock, 1);
            if (woken)
                CHECK(testCounter1 > 0);
            testCounter1++;
            
            testLock.unlock();

            testCondVar.signal();

            rendezvous();

            CHECK(testCounter1 == NUM_THREADS);

            break;
          }

          case 5: {
            // * all threads sleep with a timeout
            // * the first thread is woken up by a timeout
            // * other threads either time out or are woken up by a broadcast
            // * all threads broadcast after waking up
            testLock.lock();
       
            bool woken = testCondVar.wait(testLock, 1);
            if (woken)
                CHECK(testCounter1 > 0);
            testCounter1++;
            
            testLock.unlock();
       
            testCondVar.broadcast();
       
            rendezvous();
       
            CHECK(testCounter1 == NUM_THREADS);
       
            break;
          }
       
          case 6: {
            // * the goal of this test is to verify that one broadcast event cannot wake up the same thread twice.
            // * first N-1 threads sleep
            // * Nth thread broadcasts, switches to a secondary counter, and sleeps
            // * all threads, after waking up, switch to a secondary counter and sleep again
            // * the Nth thread broadcasts
            js::AutoMutexLock lock(testLock);
       
            if (++testCounter1 < NUM_THREADS)
                testCondVar.wait(testLock);
            else
                testCondVar.broadcast();
       
            CHECK(testCounter1 == NUM_THREADS);
       
            if (++testCounter2 < NUM_THREADS)
                testCondVar.wait(testLock);
            else
                testCondVar.broadcast();
       
            CHECK(testCounter2 == NUM_THREADS);

            break;
          }
       
          case 7: {
            // * first N/2-1 threads th sleep without a timeout
            // * thread N/2 broadcasts
            // * the remaining threads time out
            const int half = NUM_THREADS >> 1;
            
            testLock.lock();
       
            testCounter1++;
            if (testCounter1 < half) {
                testCondVar.wait(testLock);
                testCounter2++;
            } else if (testCounter1 == half) {
                testCondVar.broadcast();
            } else {
                bool woken = testCondVar.wait(testLock, 1);
                CHECK(!woken);
            }
       
            testLock.unlock();
       
            rendezvous();
       
            CHECK(testCounter1 == NUM_THREADS);
            CHECK(testCounter2 == half - 1);

            break;
          }

          default:
            static_assert(NUM_TESTS > 7, "All tests are included");
            CHECK(false);
        }
        
        // Move to the next test or the next round.
        if (++currentTest >= NUM_TESTS) {
            currentTest = 0;
            currentRound++;
        }
    }

    return true;
}


bool ConditionVariableTest::rendezvous() {
    static volatile int rendezvousCounter = 0;
    js::AutoMutexLock lock(rendezvousLock);

    if (++rendezvousCounter < NUM_THREADS) {
        rendezvousCondVar.wait(rendezvousLock);

    } else {
        rendezvousCounter = 0;
        rendezvousCondVar.broadcast();
    }

    return true;
}

static ConditionVariableTest testConditionVariable;
