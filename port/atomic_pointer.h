// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// AtomicPointer provides storage for a lock-free pointer.
// Platform-dependent implementation of AtomicPointer:
// - If the platform provides a cheap barrier, we use it with raw pointers
// - If <atomic> is present (on newer versions of gcc, it is), we use
//   a <atomic>-based AtomicPointer.  However we prefer the memory
//   barrier based version, because at least on a gcc 4.4 32-bit build
//   on linux, we have encountered a buggy <atomic> implementation.
//   Also, some <atomic> implementations are much slower than a memory-barrier
//   based implementation (~16ns for <atomic> based acquire-load vs. ~1ns for
//   a barrier based acquire-load).
// This code is based on atomicops-internals-* in Google's perftools:
// http://code.google.com/p/google-perftools/source/browse/#svn%2Ftrunk%2Fsrc%2Fbase

#ifndef PORT_ATOMIC_POINTER_H_
#define PORT_ATOMIC_POINTER_H_

#include <stdint.h>

#include <atomic>

#ifdef OS_WIN
#include <windows.h>
#endif

#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_CPU_X86_FAMILY 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
#define ARCH_CPU_X86_FAMILY 1
#elif defined(__ARMEL__)
#define ARCH_CPU_ARM_FAMILY 1
#elif defined(__aarch64__)
#define ARCH_CPU_ARM64_FAMILY 1
#elif defined(__ppc__) || defined(__powerpc__) || defined(__powerpc64__)
#define ARCH_CPU_PPC_FAMILY 1
#elif defined(__mips__)
#define ARCH_CPU_MIPS_FAMILY 1
#endif

namespace leveldb {
    namespace port {

// Define MemoryBarrier() if available
// Windows on x86
#if defined(OS_WIN) && defined(COMPILER_MSVC) && defined(ARCH_CPU_X86_FAMILY)
        // windows.h already provides a MemoryBarrier(void) macro
        // http://msdn.microsoft.com/en-us/library/ms684208(v=vs.85).aspx
#define LEVELDB_HAVE_MEMORY_BARRIER

        // Mac OS
#elif defined(__APPLE__)

        inline void MemoryBarrier() {
            std::atomic_thread_fence(std::memory_order_seq_cst);
        }

#define LEVELDB_HAVE_MEMORY_BARRIER

// Gcc on x86
#elif defined(ARCH_CPU_X86_FAMILY) && defined(__GNUC__)
        inline void MemoryBarrier() {
                  // See http://gcc.gnu.org/ml/gcc/2003-04/msg01180.html for a discussion on
                  // this idiom. Also see http://en.wikipedia.org/wiki/Memory_ordering.
                  __asm__ __volatile__("" : : : "memory");
                }
        #define LEVELDB_HAVE_MEMORY_BARRIER

                // Sun Studio

#endif

// AtomicPointer built using platform-specific MemoryBarrier().
#if defined(LEVELDB_HAVE_MEMORY_BARRIER)

        class AtomicPointer {
        private:
            void *rep_;
        public:
            AtomicPointer() {}

            explicit AtomicPointer(void *p) : rep_(p) {}

            inline void *NoBarrier_Load() const { return rep_; }

            inline void NoBarrier_Store(void *v) { rep_ = v; }

            inline void *Acquire_Load() const {
                void *result = rep_;
                MemoryBarrier();
                return result;
            }

            inline void Release_Store(void *v) {
                MemoryBarrier();
                rep_ = v;
            }
        };

// AtomicPointer based on C++11 <atomic>.
#else
        class AtomicPointer {
         private:
          std::atomic<void*> rep_;
         public:
          AtomicPointer() { }
          explicit AtomicPointer(void* v) : rep_(v) { }
          inline void* Acquire_Load() const {
            return rep_.load(std::memory_order_acquire);
          }
          inline void Release_Store(void* v) {
            rep_.store(v, std::memory_order_release);
          }
          inline void* NoBarrier_Load() const {
            return rep_.load(std::memory_order_relaxed);
          }
          inline void NoBarrier_Store(void* v) {
            rep_.store(v, std::memory_order_relaxed);
          }
        };

#endif

#undef LEVELDB_HAVE_MEMORY_BARRIER
#undef ARCH_CPU_X86_FAMILY
#undef ARCH_CPU_ARM_FAMILY
#undef ARCH_CPU_ARM64_FAMILY
#undef ARCH_CPU_PPC_FAMILY

    }  // namespace port
} // namespace leveldb

#endif  // PORT_ATOMIC_POINTER_H_
