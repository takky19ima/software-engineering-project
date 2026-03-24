#!/usr/bin/env python3
import os
import sys
import subprocess
import time
import tempfile
import select

def main():
    print("=== Sim Python Tester ===")
    
    # Create temp dir for pipes
    temp_dir = tempfile.mkdtemp(prefix="simtest_")
    cmd_pipe = os.path.join(temp_dir, "cmd.pipe")
    data_pipe = os.path.join(temp_dir, "data.pipe")
    
    os.mkfifo(cmd_pipe)
    os.mkfifo(data_pipe)
    
    # Start simulator
    cmd = ["./sim", "--cmd-pipe", cmd_pipe, "--data-pipe", data_pipe, 
           "Worlds/tiny.world", "Bugs/simple.bug", "Bugs/simple.bug"]
    
    print(f"Starting: {' '.join(cmd)}")
    with open("sim_stdout.log", "w") as out_log, open("sim_stderr.log", "w") as err_log:
        sim = subprocess.Popen(cmd, stdout=out_log, stderr=err_log)
    
    time.sleep(1)
    if sim.poll() is not None:
        print(f"Simulator crashed immediately! Exit code: {sim.returncode}")
        return
        
    print(f"Simulator running (PID: {sim.pid})")
    
    # Open pipes (data first to avoid blocking simulator)
    print("Opening data pipe...")
    data_fd = os.open(data_pipe, os.O_RDONLY | os.O_NONBLOCK)
    
    print("Opening cmd pipe...")
    
    attempts = 0
    cmd_fd = -1
    while attempts < 50:
        try:
            cmd_fd = os.open(cmd_pipe, os.O_WRONLY | os.O_NONBLOCK)
            break
        except OSError:
            time.sleep(0.1)
            attempts += 1
            if sim.poll() is not None:
                print(f"Simulator died while opening cmd pipe. Code: {sim.returncode}")
                return
                
    if cmd_fd == -1:
        print("Timeout opening cmd pipe")
        sim.kill()
        return
        
    print("Pipes connected.")
    
    # Send STEP command
    step_cmd = b"STEP 1\n"
    print(f"Writing: {step_cmd.decode('utf-8').strip()}")
    os.write(cmd_fd, step_cmd)
    
    # Read response
    print("Waiting for response...")
    response = b""
    start_time = time.time()
    
    while time.time() - start_time < 5.0:
        r, _, _ = select.select([data_fd], [], [], 1.0)
        if data_fd in r:
            chunk = os.read(data_fd, 4096)
            if not chunk:
                print("Data pipe closed (EOF)")
                break
            response += chunk
            print(f"Read {len(chunk)} bytes")
            if b"END\n" in chunk or b"END\r\n" in chunk:
                print("Received END marker")
                break
        
        # Check if sim died
        if sim.poll() is not None:
            print(f"Simulator died while waiting for data! Exit code: {sim.returncode}")
            break
            
    print("\n--- Response snippet ---")
    resp_str = response.decode('utf-8', errors='replace')
    print(resp_str[:500] + ("..." if len(resp_str) > 500 else ""))
    print("------------------------")
    
    sim.kill()
    sim.wait()
    os.close(cmd_fd)
    os.close(data_fd)
    
    # Clean up pipes
    os.unlink(cmd_pipe)
    os.unlink(data_pipe)
    os.rmdir(temp_dir)
    
    print("\nChecking stderr log:")
    with open("sim_stderr.log", "r") as f:
        err = f.read()
        if err:
            print(err)
        else:
            print("(empty)")

if __name__ == "__main__":
    main()
