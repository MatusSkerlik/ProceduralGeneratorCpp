import subprocess
import argparse
import os
import time
import threading
import signal

m_times = None
modification_running = True
execution_running = True
spawner_running = True
despawner_running = True
execute_event = threading.Event()
spawn_event = threading.Event()
despawn_event = threading.Event()
process = None

def check_modification():
    while modification_running:
        should_set = False
        for file in m_times:
            cm_time = os.path.getmtime(file)
            if cm_time != m_times[file]:
                m_times[file] = cm_time
                should_set = True
        if should_set:
            despawn_event.set()
        time.sleep(1/20)

def execute_command(command):
    while execution_running:
        execute_event.wait()
        execute_event.clear()
        if execution_running:
            subprocess.run(command)
        spawn_event.set()

def despawn_process():
    global process
    while despawner_running:
        despawn_event.wait()
        despawn_event.clear()
        if process is not None:
            process.terminate()
            process = None
        execute_event.set()

def spawn_process(program):
    global process
    if program:
        while spawner_running:
            spawn_event.wait()
            spawn_event.clear()
            if spawner_running:
                process = subprocess.Popen(program)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("FILES", nargs="+", help="Specify which files should be tracked for change.")
    parser.add_argument("-x", "--execute", help="Execute command on file change.", required=True)
    parser.add_argument("-r", "--run", help="Specify process to run after command execution.", required=False)
    parser.exit_on_error = True
    args = parser.parse_args()

    # SETUP
    try:
        m_times = {file:os.path.getmtime(file) for file in args.FILES}
    except FileNotFoundError as e:
        raise SystemExit(e) 
    
    execute_event.clear()
    spawn_event.clear()
    despawn_event.clear()

    modification_runner = threading.Thread(target=check_modification) 
    execution_runner = threading.Thread(target=execute_command, args=(args.execute,))
    spawn_runner = threading.Thread(target=spawn_process, args=(args.run,))
    despawn_runner = threading.Thread(target=despawn_process)

    # RUN
    print("Watching modification of files:")
    for file in args.FILES:
        print("    - %s," % file)
    print("after modification executes command: '%s'." % args.execute)

    modification_runner.start()
    execution_runner.start()
    spawn_runner.start()
    despawn_runner.start()
    
    running = True
    while running:
        try:
            time.sleep(1/20)
        except KeyboardInterrupt:
            modification_running = False
            execution_running = False
            spawner_running = False
            despawner_running = False
            spawn_event.set()
            execute_event.set()
            despawn_event.set()
            modification_runner.join()
            execution_runner.join()
            spawn_runner.join()
            despawn_runner.join()
            running = False
