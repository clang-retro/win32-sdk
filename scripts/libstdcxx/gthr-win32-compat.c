/* The thread functions of libstdc++ (GCC's win32 thread model) for every
   32-bit Windows: 95, 98, ME, Win32s, NT 3.1 and later.

   GCC's own functions (libgcc/config/i386/gthr-win32*.c) need Windows
   Vista: condition variables, GetThreadId and TryEnterCriticalSection.
   build-libstdcxx.sh puts this file in libstdc++.a in their place for a
   sysroot below Vista (_WIN32_WINNT below 0x0600). It has the same names,
   so the libstdc++ headers call it as they call GCC's.

   - A condition variable is a queue of waiting threads, each with its own
     event. A signal wakes exactly one thread that was waiting when it
     came. The queue is made on first use and kept where the pointer of a
     CONDITION_VARIABLE is (__gthread_cond_t).
   - For 95, 98, ME and NT 4.0 (_WIN32_WINNT below 0x0500), which have no
     TryEnterCriticalSection, a mutex is a count of the threads that want
     it and a semaphore that they wait on, with its owner and depth for a
     recursive mutex. It is kept where the CRITICAL_SECTION is
     (__gthread_mutex_t). From 2000, a mutex is a CRITICAL_SECTION, as in
     GCC.
   - A thread is the HANDLE from CreateThread. Two threads are equal when
     their handles are equal: __gthr_win32_self gives the same handle as
     __gthr_win32_create.

   The same design as the libc++ threads of clang-retro/llvm
   (libcxx/src/support/win32/thread_win32.cpp).

   This file is part of clang-retro/win32-sdk. It is under the GPL 3 with
   the GCC Runtime Library Exception, as the libgcc files it replaces.  */

#define __GTHREAD_HIDE_WIN32API 1
#define __GTHREAD_WIN32_COMPAT 1
#include "gthr-win32.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <errno.h>
#include <stdlib.h>

#if _WIN32_WINNT < 0x0500
#define COUNT_MUTEX 1
#else
#define COUNT_MUTEX 0
#endif

/* Check the sizes of the local version of the Win32 types.  */
#define CHECK_SIZE_OF(TYPE) \
  typedef int assertion_##TYPE[sizeof (__gthr_win32_##TYPE) == sizeof (TYPE) ? 1 : -1];
CHECK_SIZE_OF (DWORD)
CHECK_SIZE_OF (HANDLE)
CHECK_SIZE_OF (CRITICAL_SECTION)

/* Threads.  */

/* The descriptor of a thread, kept in its TLS slot.  */
struct thr_desc
{
  void *(*func) (void *);
  void *args;
  HANDLE h;
};

static __gthread_key_t thr_tls = TLS_OUT_OF_INDEXES;
static __gthread_once_t thr_tls_once = __GTHREAD_ONCE_INIT;

/* Makes the TLS slot for the thread descriptors.  */
static void
thr_tls_init (void)
{
  if (__gthr_win32_key_create (&thr_tls, free))
    abort ();
}

/* Runs the function of a thread from __gthr_win32_create.  */
static DWORD WINAPI
thr_wrapper (LPVOID args)
{
  struct thr_desc *td = (struct thr_desc *) args;
  __gthr_win32_setspecific (thr_tls, td);
  return (DWORD) (ULONG_PTR) (*td->func) (td->args);
}

/* Starts a thread that runs FUNC (ARGS), and gives its handle in THR.  */
int
__gthr_win32_create (__gthread_t *thr, void *(*func) (void *), void *args)
{
  struct thr_desc *td;
  DWORD id; /* 95, 98 and ME need a place for the thread ID.  */

  __gthr_win32_once (&thr_tls_once, thr_tls_init);
  td = malloc (sizeof (struct thr_desc));
  if (!td)
    return ENOMEM;
  td->func = func;
  td->args = args;
  td->h = CreateThread (NULL, 0, thr_wrapper, td, CREATE_SUSPENDED, &id);
  if (!td->h)
    {
      free (td);
      return (int) GetLastError ();
    }
  *thr = (__gthread_t) td->h;
  ResumeThread (td->h);
  return 0;
}

/* Waits for the thread THR to end, gives its result in VALUE_PTR and
   closes its handle.  */
int
__gthr_win32_join (__gthread_t thr, void **value_ptr)
{
  int status = 0;

  if (thr == __gthr_win32_self ())
    return EDEADLK;
  if (WaitForSingleObject ((HANDLE) thr, INFINITE) == WAIT_OBJECT_0)
    {
      DWORD exit_code;
      if (!value_ptr)
	;
      else if (GetExitCodeThread ((HANDLE) thr, &exit_code))
	*value_ptr = (void *) (ULONG_PTR) exit_code;
      else
	status = (int) GetLastError ();
    }
  else
    status = (int) GetLastError ();
  CloseHandle ((HANDLE) thr);
  return status;
}

/* Gives the handle of the thread that calls it. For a thread that
   __gthr_win32_create did not start, the handle is made on the first
   call.  */
__gthread_t
__gthr_win32_self (void)
{
  struct thr_desc *td;

  __gthr_win32_once (&thr_tls_once, thr_tls_init);
  td = __gthr_win32_getspecific (thr_tls);
  if (!td)
    {
      HANDLE proc = GetCurrentProcess ();
      td = malloc (sizeof (struct thr_desc));
      if (!td)
	abort ();
      td->func = NULL;
      td->args = NULL;
      if (!DuplicateHandle (proc, GetCurrentThread (), proc, &td->h, 0,
			    FALSE, DUPLICATE_SAME_ACCESS))
	abort ();
      __gthr_win32_setspecific (thr_tls, td);
    }
  return (__gthread_t) td->h;
}

/* Lets the thread THR end without a join.  */
int
__gthr_win32_detach (__gthread_t thr)
{
  CloseHandle ((HANDLE) thr);
  return 0;
}

/* Tells if T1 and T2 are the same thread.  */
int
__gthr_win32_equal (__gthread_t t1, __gthread_t t2)
{
  return t1 == t2;
}

/* Gives the rest of the time slice to other threads. 95, 98 and ME have
   no SwitchToThread.  */
int
__gthr_win32_yield (void)
{
  Sleep (0);
  return 0;
}

/* Runs FUNC once, the first time that a thread calls it with ONCE. The
   other threads wait until it is done.  */
int
__gthr_win32_once (__gthread_once_t *once, void (*func) (void))
{
  if (__builtin_expect (!__atomic_load_n (&once->done, __ATOMIC_ACQUIRE), 0))
    {
      if (__atomic_add_fetch (&once->started, 1, __ATOMIC_ACQ_REL) == 0)
	{
	  (*func) ();
	  __atomic_store_n (&once->done, 1, __ATOMIC_RELEASE);
	}
      else
	while (!__atomic_load_n (&once->done, __ATOMIC_ACQUIRE))
	  Sleep (0);
    }
  return 0;
}

/* Thread-local keys. The C runtime of mingw-w64 runs DTOR for the value
   of each thread when it ends.  */
int
__gthr_win32_key_create (__gthread_key_t *key, void (*dtor) (void *))
{
  DWORD index = TlsAlloc ();
  if (index == TLS_OUT_OF_INDEXES)
    return (int) GetLastError ();
  *key = index;
#ifdef MINGW32_SUPPORTS_MT_EH
  return __mingwthr_key_dtor (*key, dtor);
#else
  (void) dtor;
  return 0;
#endif
}

/* Frees the thread-local key KEY.  */
int
__gthr_win32_key_delete (__gthread_key_t key)
{
  return TlsFree (key) ? 0 : (int) GetLastError ();
}

/* Gives the value of the key KEY for this thread. It keeps the last
   error.  */
void *
__gthr_win32_getspecific (__gthread_key_t key)
{
  DWORD last_error = GetLastError ();
  void *ptr = TlsGetValue (key);
  SetLastError (last_error);
  return ptr;
}

/* Sets the value of the key KEY for this thread.  */
int
__gthr_win32_setspecific (__gthread_key_t key, const void *ptr)
{
  return TlsSetValue (key, (void *) ptr) ? 0 : (int) GetLastError ();
}

/* Mutexes.  */

#if COUNT_MUTEX

/* A mutex for 95, 98, ME and NT 4.0. Taking it adds one to COUNT; a
   thread that finds it above zero waits for the semaphore, which the
   owner releases when it leaves with others still counted.  */
struct count_mutex
{
  LONG count;
  HANDLE sem;
  DWORD owner;
  LONG depth;
};
typedef int assertion_count_mutex[sizeof (struct count_mutex)
				  <= sizeof (__gthread_mutex_t) ? 1 : -1];

/* Gives the count mutex that is kept in M.  */
static struct count_mutex *
count_mutex (__gthread_mutex_t *m)
{
  return (struct count_mutex *) m;
}

/* Makes the mutex M.  */
void
__gthr_win32_mutex_init_function (__gthread_mutex_t *m)
{
  struct count_mutex *c = count_mutex (m);
  c->count = 0;
  c->owner = 0;
  c->depth = 0;
  c->sem = CreateSemaphoreA (NULL, 0, 0x7fffffff, NULL);
  if (!c->sem)
    abort ();
}

/* Frees the mutex M.  */
void
__gthr_win32_mutex_destroy (__gthread_mutex_t *m)
{
  CloseHandle (count_mutex (m)->sem);
}

/* Takes the mutex M, again if this thread has it (recursive mutex).  */
int
__gthr_win32_mutex_lock (__gthread_mutex_t *m)
{
  struct count_mutex *c = count_mutex (m);
  DWORD self = GetCurrentThreadId ();

  if (__atomic_load_n (&c->owner, __ATOMIC_RELAXED) == self)
    {
      ++c->depth;
      return 0;
    }
  if (__atomic_fetch_add (&c->count, 1, __ATOMIC_ACQ_REL) > 0)
    WaitForSingleObject (c->sem, INFINITE);
  __atomic_store_n (&c->owner, self, __ATOMIC_RELAXED);
  c->depth = 1;
  return 0;
}

/* Takes the mutex M if no thread has it.  */
static int
count_mutex_try (struct count_mutex *c)
{
  LONG expected = 0;
  if (!__atomic_compare_exchange_n (&c->count, &expected, 1, 0,
				    __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
    return 1;
  __atomic_store_n (&c->owner, GetCurrentThreadId (), __ATOMIC_RELAXED);
  c->depth = 1;
  return 0;
}

/* Takes the mutex M if no thread has it. Gives 0 if it took it.  */
int
__gthr_win32_mutex_trylock (__gthread_mutex_t *m)
{
  return count_mutex_try (count_mutex (m));
}

/* Takes the recursive mutex M if no other thread has it.  */
int
__gthr_win32_recursive_mutex_trylock (__gthread_recursive_mutex_t *m)
{
  struct count_mutex *c = count_mutex (m);
  if (__atomic_load_n (&c->owner, __ATOMIC_RELAXED) == GetCurrentThreadId ())
    {
      ++c->depth;
      return 0;
    }
  return count_mutex_try (c);
}

/* Releases the mutex M once.  */
int
__gthr_win32_mutex_unlock (__gthread_mutex_t *m)
{
  struct count_mutex *c = count_mutex (m);
  if (--c->depth > 0)
    return 0;
  __atomic_store_n (&c->owner, 0, __ATOMIC_RELAXED);
  if (__atomic_fetch_sub (&c->count, 1, __ATOMIC_ACQ_REL) > 1)
    ReleaseSemaphore (c->sem, 1, NULL);
  return 0;
}

#else /* !COUNT_MUTEX */

/* Makes the mutex M.  */
void
__gthr_win32_mutex_init_function (__gthread_mutex_t *m)
{
  InitializeCriticalSection ((LPCRITICAL_SECTION) m);
}

/* Frees the mutex M.  */
void
__gthr_win32_mutex_destroy (__gthread_mutex_t *m)
{
  DeleteCriticalSection ((LPCRITICAL_SECTION) m);
}

/* Takes the mutex M, again if this thread has it (recursive mutex).  */
int
__gthr_win32_mutex_lock (__gthread_mutex_t *m)
{
  EnterCriticalSection ((LPCRITICAL_SECTION) m);
  return 0;
}

/* Takes the mutex M if no thread has it. Gives 0 if it took it.  */
int
__gthr_win32_mutex_trylock (__gthread_mutex_t *m)
{
  if (!TryEnterCriticalSection ((LPCRITICAL_SECTION) m))
    return 1;
  if (m->RecursionCount > 1)
    {
      LeaveCriticalSection ((LPCRITICAL_SECTION) m);
      return 1;
    }
  return 0;
}

/* Takes the recursive mutex M if no other thread has it.  */
int
__gthr_win32_recursive_mutex_trylock (__gthread_recursive_mutex_t *m)
{
  return TryEnterCriticalSection ((LPCRITICAL_SECTION) m) ? 0 : 1;
}

/* Releases the mutex M once.  */
int
__gthr_win32_mutex_unlock (__gthread_mutex_t *m)
{
  LeaveCriticalSection ((LPCRITICAL_SECTION) m);
  return 0;
}

#endif /* COUNT_MUTEX */

/* Condition variables.  */

/* A thread that waits on a condition variable.  */
struct cv_waiter
{
  HANDLE event;
  int signaled;
  struct cv_waiter *next;
};

/* The queue of the waiting threads of a condition variable.  */
struct cv_state
{
  CRITICAL_SECTION lock;
  struct cv_waiter *head;
  struct cv_waiter *tail;
};

/* Gives the queue of the condition variable CV, made on the first call.
   95 has no InterlockedCompareExchange; the compiler's atomic built-in
   does the same.  */
static struct cv_state *
cv_state (__gthread_cond_t *cv)
{
  struct cv_state *st = __atomic_load_n ((struct cv_state **) &cv->Ptr,
					 __ATOMIC_ACQUIRE);
  struct cv_state *expected = NULL;

  if (st)
    return st;
  st = malloc (sizeof (struct cv_state));
  if (!st)
    abort ();
  InitializeCriticalSection (&st->lock);
  st->head = st->tail = NULL;
  if (__atomic_compare_exchange_n ((struct cv_state **) &cv->Ptr, &expected,
				   st, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
    return st;
  /* Another thread made it first.  */
  DeleteCriticalSection (&st->lock);
  free (st);
  return expected;
}

/* Makes the condition variable CV. The queue is made on first use.  */
void
__gthr_win32_cond_init_function (__gthread_cond_t *cv)
{
  cv->Ptr = NULL;
}

/* Frees the queue of the condition variable CV. No thread waits on it.  */
int
__gthr_win32_cond_destroy (__gthread_cond_t *cv)
{
  struct cv_state *st = (struct cv_state *) cv->Ptr;
  if (st)
    {
      DeleteCriticalSection (&st->lock);
      free (st);
      cv->Ptr = NULL;
    }
  return 0;
}

/* Wakes the first waiting thread of CV, or each one if ALL. The event is
   set with the lock held: the waiter closes it after it sees SIGNALED
   under the same lock.  */
static void
cv_wake (__gthread_cond_t *cv, int all)
{
  struct cv_state *st;
  struct cv_waiter *w;

  if (!__atomic_load_n (&cv->Ptr, __ATOMIC_ACQUIRE))
    return; /* No thread waited.  */
  st = cv_state (cv);
  EnterCriticalSection (&st->lock);
  while ((w = st->head) != NULL)
    {
      st->head = w->next;
      if (!st->head)
	st->tail = NULL;
      w->signaled = 1;
      SetEvent (w->event);
      if (!all)
	break;
    }
  LeaveCriticalSection (&st->lock);
}

/* Wakes each thread that waits on CV.  */
int
__gthr_win32_cond_broadcast (__gthread_cond_t *cv)
{
  cv_wake (cv, 1);
  return 0;
}

/* Wakes one thread that waits on CV.  */
int
__gthr_win32_cond_signal (__gthread_cond_t *cv)
{
  cv_wake (cv, 0);
  return 0;
}

/* Releases the mutex M, waits on CV for at most MS milliseconds, and takes
   M again. Gives ETIMEDOUT if no signal came.  */
static int
cv_wait (__gthread_cond_t *cv, __gthread_mutex_t *m, DWORD ms)
{
  struct cv_state *st = cv_state (cv);
  struct cv_waiter w;
  int timed_out;

  w.event = CreateEventA (NULL, FALSE, FALSE, NULL);
  if (!w.event)
    return (int) GetLastError ();
  w.signaled = 0;
  w.next = NULL;

  EnterCriticalSection (&st->lock);
  if (st->tail)
    st->tail->next = &w;
  else
    st->head = &w;
  st->tail = &w;
  LeaveCriticalSection (&st->lock);

  __gthr_win32_mutex_unlock (m);
  WaitForSingleObject (w.event, ms);

  /* Signaled or not, take the waiter off the queue before it goes away.  */
  EnterCriticalSection (&st->lock);
  timed_out = !w.signaled;
  if (timed_out)
    {
      struct cv_waiter *prev = NULL, *i;
      for (i = st->head; i; prev = i, i = i->next)
	if (i == &w)
	  {
	    if (prev)
	      prev->next = w.next;
	    else
	      st->head = w.next;
	    if (st->tail == &w)
	      st->tail = prev;
	    break;
	  }
    }
  LeaveCriticalSection (&st->lock);
  CloseHandle (w.event);

  __gthr_win32_mutex_lock (m);
  return timed_out ? ETIMEDOUT : 0;
}

/* Releases the mutex M, waits on CV and takes M again.  */
int
__gthr_win32_cond_wait (__gthread_cond_t *cv, __gthread_mutex_t *m)
{
  return cv_wait (cv, m, INFINITE);
}

/* The number of 100-nanoseconds between 1/1/1601 and 1/1/1970.  */
#define FILETIME_1970 116444736000000000ULL

/* Gives the time from now until ABS_TIME (in the time of the system
   clock, from 1/1/1970) in milliseconds, or 0 if it is past. NT 3.1 and
   Win32s have no GetSystemTimeAsFileTime.  */
DWORD
__gthr_win32_abs_to_rel_time (const __gthread_time_t *abs_time)
{
  union
  {
    ULONGLONG nsec100;
    FILETIME ft;
  } now;
  SYSTEMTIME st;
  ULONGLONG abs_nsec100, ms;

  GetSystemTime (&st);
  SystemTimeToFileTime (&st, &now.ft);
  now.nsec100 -= FILETIME_1970;
  abs_nsec100 = (ULONGLONG) abs_time->tv_sec * 10000000ULL
		+ (abs_time->tv_nsec + 99) / 100;
  if (abs_nsec100 < now.nsec100)
    return 0;
  ms = (abs_nsec100 - now.nsec100 + 9999) / 10000;
  return ms < INFINITE ? (DWORD) ms : INFINITE - 1;
}

/* Releases the mutex M, waits on CV until ABS_TIME, and takes M again.
   Gives ETIMEDOUT if no signal came.  */
int
__gthr_win32_cond_timedwait (__gthread_cond_t *cv, __gthread_mutex_t *m,
			     const __gthread_time_t *abs_time)
{
  return cv_wait (cv, m, __gthr_win32_abs_to_rel_time (abs_time));
}
