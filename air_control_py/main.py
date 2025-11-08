import ctypes
import mmap
import os
import signal
import subprocess
import threading
import time
#import sys //testing

_libc = ctypes.CDLL(None, use_errno=True)

# shm_open(const char *name, int oflag, mode_t mode)
_libc.shm_open.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int]
_libc.shm_open.restype = ctypes.c_int

# ftruncate(int fd, off_t length)
_libc.ftruncate.argtypes = [ctypes.c_int, ctypes.c_long]
_libc.ftruncate.restype = ctypes.c_int

# shm_unlink(const char *name)
_libc.shm_unlink.argtypes = [ctypes.c_char_p]
_libc.shm_unlink.restype = ctypes.c_int



TOTAL_TAKEOFFS = 20
STRIPS = 5
SHM_NAME = b"/shm_pids"
RADIO_PID=0
shm_data = []

# TODO1: Size of shared memory for 3 integers (current process pid, radio, ground) use ctypes.sizeof()
SHM_LENGTH =ctypes.sizeof(ctypes.c_int) * 3

# Global variables and locks
planes = 0  # planes waiting
takeoffs = 0  # local takeoffs (per thread)
total_takeoffs = 0  # total takeoffs

state_lock = threading.Lock()
runway1_lock = threading.Lock()
runway2_lock = threading.Lock()

terminate_sent = False



def create_shared_memory():
    """Create shared memory segment for PID exchange"""
    # TODO 6:
    # 1. Encode (utf-8) the shared memory name to use with shm_open
    name_b = SHM_NAME if isinstance(SHM_NAME, (bytes, bytearray)) else SHM_NAME.encode("utf-8")

    # 2. Temporarily adjust the permission mask (umask) so the memory can be created with appropriate permissions
    old_umask = os.umask(0)
    try:
        try:
           _libc.shm_unlink(name_b)
        except Exception:
            pass
    # 3. Use _libc.shm_open to create the shared memory
        fd = _libc.shm_open(name_b, os.O_CREAT | os.O_RDWR | os.O_EXCL, 0o666)
        if fd < 0:
            raise OSError(ctypes.get_errno(), "shm_open failed")
    # 4. Use _libc.ftruncate to set the size of the shared memory (SHM_LENGTH)
        if _libc.ftruncate(fd, SHM_LENGTH) != 0:
            raise OSError(ctypes.get_errno(), "ftruncate failed")
    finally:
     # 5. Restore the original permission mask (umask)  
        os.umask(old_umask) 

    # 6. Use mmap to map the shared memory
    mm = mmap.mmap(fd, SHM_LENGTH, flags=mmap.MAP_SHARED, prot=mmap.PROT_READ | mmap.PROT_WRITE)
    # 7. Create an integer-array view (use memoryview()) to access the shared memory
    view = memoryview(mm).cast('i')
    # 8. Return the file descriptor (shm_open), mmap object and memory view
    return fd, mm, view


def HandleUSR2(signum, frame):
    """Handle external signal indicating arrival of 5 new planes.
    Complete function to update waiting planes"""
    global planes
    # TODO 4: increment the global variable planes
    with state_lock:
        planes +=5


def TakeOffFunction(agent_id: int):
    """Function executed by each THREAD to control takeoffs.
    Complete using runway1_lock and runway2_lock and state_lock to synchronize"""
    global planes, takeoffs, total_takeoffs, RADIO_PID, terminate_sent

    # TODO: implement the logic to control a takeoff thread


    # Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
    while total_takeoffs < TOTAL_TAKEOFFS:

    # Use runway1_lock or runway2_lock to simulate runway being locked
        active_runway = None
        if runway1_lock.acquire(blocking=False):
            active_runway = runway1_lock
        elif runway2_lock.acquire(blocking=False):
            active_runway = runway2_lock
        else:
            time.sleep(0.001)
            continue

        did_takeoff=False


    # Use state_lock for safe access to shared variables (planes, takeoffs, total_takeoffs)
        with state_lock:
            if planes > 0:
                planes -= 1
                takeoffs += 1
                total_takeoffs += 1
                did_takeoff=True

    
    # Send SIGUSR1 every 5 local takeoffs

            if takeoffs == 5 and RADIO_PID>0:
                try:
                    os.kill(RADIO_PID, signal.SIGUSR1)
                except ProcessLookupError:
                    pass
                takeoffs = 0
    # Send SIGTERM when the total takeoffs target is reached
            if total_takeoffs>= TOTAL_TAKEOFFS and not terminate_sent and RADIO_PID>0:
                os.kill(RADIO_PID, signal.SIGTERM)
                terminate_sent=True 
    # Simulate the time a takeoff takes with sleep(1)  
        if did_takeoff:
            time.sleep(1)


        active_runway.release()
        if terminate_sent:
            break
    


    


def launch_radio():
    """unblock the SIGUSR2 signal so the child receives it"""
    signal.pthread_sigmask(signal.SIG_UNBLOCK, {signal.SIGUSR2})
    

    # TODO 8: Launch the external 'radio' process using subprocess.Popen()
    process = subprocess.Popen(["./radio", SHM_NAME.decode("utf-8")])
    return process

    # """Test-only version: log incoming signals to radio_signals.log"""
    # try:
    #     signal.pthread_sigmask(signal.SIG_UNBLOCK, {signal.SIGUSR2})
    # except AttributeError:
    #     pass

    # script = (
    #     "import signal,sys,time\n"
    #     "log=open('radio_signals.log','w')\n"
    #     "def on_usr1(*a): log.write('SIGUSR1\\n'); log.flush()\n"
    #     "def on_term(*a): log.write('SIGTERM\\n'); log.flush(); sys.exit(0)\n"
    #     "signal.signal(signal.SIGUSR1,on_usr1)\n"
    #     "signal.signal(signal.SIGTERM,on_term)\n"
    #     "while True: time.sleep(1)\n"
    # )
    # return subprocess.Popen([sys.executable, '-u', '-c', script]) 


def main():
    global shm_data, RADIO_PID

    # TODO 2: set the handler for the SIGUSR2 signal to HandleUSR2
    signal.signal(signal.SIGUSR2, HandleUSR2)


    # TODO 5: Create the shared memory and store the current process PID using create_shared_memory()
    fd, memory, data = create_shared_memory()
    data[0]=os.getpid()
    data[2]=0
    # TODO 7: Run radio and store its PID in shared memory, use the launch_radio function
    radio_process = launch_radio()
    RADIO_PID = radio_process.pid
    data[1] = RADIO_PID
    # TODO 9: Create and start takeoff controller threads (STRIPS) 
    threads = []
    for i in range(STRIPS):
        t = threading.Thread(target=TakeOffFunction, args=(i,), daemon=True)
        t.start()
        threads.append(t)
    # TODO 10: Wait for all threads to finish their work
    time.sleep(1)
    '''    # Test-only code to inject planes via signals'''
    # for _ in range(4):  # 4 × SIGUSR2 = 20 planes total
    #     os.kill(os.getpid(), signal.SIGUSR2)
    #     time.sleep(0.1)
    # print("Injected planes, waiting on joins...", planes, total_takeoffs) 


    for t in threads:
        t.join()
    # TODO 11: Release shared memory and close resources
    try:
        memory.close()      # unmap
    except Exception:
        pass
    try:
        os.close(fd)        # close file descriptor
    except Exception:
        pass
    try:
        _libc.shm_unlink(SHM_NAME)   # remove the shared-memory object
    except Exception:
        pass



if __name__ == "__main__":
    main()