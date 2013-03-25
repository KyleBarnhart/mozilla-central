// Copyright (c) 2006-2011 The Chromium Authors. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
//  * Neither the name of Google, Inc. nor the names of its contributors
//    may be used to endorse or promote products derived from this
//    software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#ifndef TOOLS_PLATFORM_H_
#define TOOLS_PLATFORM_H_

// Uncomment this line to force desktop logging
//#define SPS_FORCE_LOG

#ifdef ANDROID
#include <android/log.h>
#else
#define __android_log_print(a, ...)
#endif

#include "mozilla/StandardInteger.h"
#include "mozilla/Util.h"
#include "mozilla/unused.h"
#include "mozilla/TimeStamp.h"
#include "PlatformMacros.h"
#include "v8-support.h"
#include <vector>
#define ASSERT(a) MOZ_ASSERT(a)
#ifdef ANDROID
#if defined(__arm__) || defined(__thumb__)
#define ENABLE_SPS_LEAF_DATA
#define ENABLE_ARM_LR_SAVING
#endif
#define LOG(text) __android_log_write(ANDROID_LOG_ERROR, "Profiler", text)
#define LOGF(format, ...) __android_log_print(ANDROID_LOG_ERROR, "Profiler", format, __VA_ARGS__)
#elif defined(SPS_FORCE_LOG)
#define LOG(text) fprintf(stderr, "Profiler: %s\n", text)
#define LOGF(format, ...) fprintf(stderr, "Profiler: " format "\n", __VA_ARGS__)
#else
#define LOG(TEXT) do {} while(0)
#define LOGF(format, ...) do {} while(0)
#endif

#if defined(XP_MACOSX) || defined(XP_WIN)
#define ENABLE_SPS_LEAF_DATA
#endif

typedef uint8_t* Address;

// ----------------------------------------------------------------------------
// Mutex
//
// Mutexes are used for serializing access to non-reentrant sections of code.
// The implementations of mutex should allow for nested/recursive locking.

class Mutex {
 public:
  virtual ~Mutex() {}

  // Locks the given mutex. If the mutex is currently unlocked, it becomes
  // locked and owned by the calling thread, and immediately. If the mutex
  // is already locked by another thread, suspends the calling thread until
  // the mutex is unlocked.
  virtual int Lock() = 0;

  // Unlocks the given mutex. The mutex is assumed to be locked and owned by
  // the calling thread on entrance.
  virtual int Unlock() = 0;

  // Tries to lock the given mutex. Returns whether the mutex was
  // successfully locked.
  virtual bool TryLock() = 0;
};

// ----------------------------------------------------------------------------
// ScopedLock
//
// Stack-allocated ScopedLocks provide block-scoped locking and
// unlocking of a mutex.
class ScopedLock {
 public:
  explicit ScopedLock(Mutex* mutex): mutex_(mutex) {
    ASSERT(mutex_ != NULL);
    mutex_->Lock();
  }
  ~ScopedLock() {
    mutex_->Unlock();
  }

 private:
  Mutex* mutex_;
  DISALLOW_COPY_AND_ASSIGN(ScopedLock);
};



// ----------------------------------------------------------------------------
// OS
//
// This class has static methods for the different platform specific
// functions. Add methods here to cope with differences between the
// supported platforms.

class OS {
 public:

  // Sleep for a number of milliseconds.
  static void Sleep(const int milliseconds);

  // Factory method for creating platform dependent Mutex.
  // Please use delete to reclaim the storage for the returned Mutex.
  static Mutex* CreateMutex();

  // On supported platforms, setup a signal handler which would start
  // the profiler.
#if defined(ANDROID)
  static void RegisterStartHandler();
#else
  static void RegisterStartHandler() {}
#endif

 private:
  static const int msPerSecond = 1000;

};




// ----------------------------------------------------------------------------
// Thread
//
// Thread objects are used for creating and running threads. When the start()
// method is called the new thread starts running the run() method in the new
// thread. The Thread object should not be deallocated before the thread has
// terminated.

class Thread {
 public:
  // Create new thread.
  explicit Thread(const char* name);
  virtual ~Thread();

  // Start new thread by calling the Run() method in the new thread.
  void Start();

  void Join();

  inline const char* name() const {
    return name_;
  }

  // Abstract method for run handler.
  virtual void Run() = 0;

  // The thread name length is limited to 16 based on Linux's implementation of
  // prctl().
  static const int kMaxThreadNameLength = 16;

  class PlatformData;
  PlatformData* data() { return data_; }

 private:
  void set_name(const char *name);

  PlatformData* data_;

  char name_[kMaxThreadNameLength];
  int stack_size_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

// ----------------------------------------------------------------------------
// HAVE_NATIVE_UNWIND
//
// Pseudo backtraces are available on all platforms.  Native
// backtraces are available only on selected platforms.  Breakpad is
// the only supported native unwinder.  HAVE_NATIVE_UNWIND is set at
// build time to indicate whether native unwinding is possible on this
// platform.  The actual unwind mode currently in use is stored in
// sUnwindMode.

#undef HAVE_NATIVE_UNWIND
#if defined(MOZ_PROFILING) \
    && (defined(SPS_PLAT_amd64_linux) || defined(SPS_PLAT_arm_android) \
        || defined(SPS_PLAT_x86_linux))
# define HAVE_NATIVE_UNWIND
#endif

// ----------------------------------------------------------------------------
// Sampler
//
// A sampler periodically samples the state of the VM and optionally
// (if used for profiling) the program counter and stack pointer for
// the thread that created it.

// TickSample captures the information collected for each sample.
class TickSample {
 public:
  TickSample()
      :
        pc(NULL),
        sp(NULL),
        fp(NULL),
#ifdef ENABLE_ARM_LR_SAVING
        lr(NULL),
#endif
        function(NULL),
        context(NULL),
        frames_count(0) {}
  Address pc;  // Instruction pointer.
  Address sp;  // Stack pointer.
  Address fp;  // Frame pointer.
#ifdef ENABLE_ARM_LR_SAVING
  Address lr;  // ARM link register
#endif
  Address function;  // The last called JS function.
  void*   context;   // The context from the signal handler, if available. On
                     // Win32 this may contain the windows thread context.
  static const int kMaxFramesCount = 64;
  Address stack[kMaxFramesCount];  // Call stack.
  int frames_count;  // Number of captured frames.
  mozilla::TimeStamp timestamp;
};

class Sampler {
 public:
  // Initialize sampler.
  explicit Sampler(int interval, bool profiling);
  virtual ~Sampler();

  int interval() const { return interval_; }

  // Performs stack sampling.
  virtual void SampleStack(TickSample* sample) = 0;

  // This method is called for each sampling period with the current
  // program counter.
  virtual void Tick(TickSample* sample) = 0;

  // Request a save from a signal handler
  virtual void RequestSave() = 0;
  // Process any outstanding request outside a signal handler.
  virtual void HandleSaveRequest() = 0;

  // Start and stop sampler.
  void Start();
  void Stop();

  // Is the sampler used for profiling?
  bool IsProfiling() const { return profiling_; }

  // Whether the sampler is running (that is, consumes resources).
  bool IsActive() const { return active_; }

  // Low overhead way to stop the sampler from ticking
  bool IsPaused() const { return paused_; }
  void SetPaused(bool value) { NoBarrier_Store(&paused_, value); }

  class PlatformData;

  PlatformData* platform_data() { return data_; }

  // If we move the backtracing code into the platform files we won't
  // need to have these hacks
#ifdef XP_WIN
  // xxxehsan sucky hack :(
  static uintptr_t GetThreadHandle(PlatformData*);
#endif
#ifdef XP_MACOSX
  static pthread_t GetProfiledThread(PlatformData*);
#endif
 private:
  void SetActive(bool value) { NoBarrier_Store(&active_, value); }

  const int interval_;
  const bool profiling_;
  Atomic32 paused_;
  Atomic32 active_;
  PlatformData* data_;  // Platform specific data.
};

#endif /* ndef TOOLS_PLATFORM_H_ */
