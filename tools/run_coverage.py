
import os
import sys
import subprocess
import time
import shutil
import glob
import binascii

# Configuration
PROJECT_DIR = "test/unity-hw-tests"

def load_env(root_dir):
    env_path = os.path.join(root_dir, ".env")
    if os.path.exists(env_path):
        print(f"Loading environment from {env_path}")
        with open(env_path, 'r') as f:
            for line in f:
                if line.strip() and not line.startswith('#'):
                    key, value = line.strip().split('=', 1)
                    if key not in os.environ:
                        os.environ[key] = value.strip('"').strip("'")


import os
import sys
import subprocess
import time
import shutil
import glob
import binascii
import argparse

# Configuration
PROJECT_DIR = "test/unity-hw-tests"

def load_env(root_dir):
    env_path = os.path.join(root_dir, ".env")
    if os.path.exists(env_path):
        print(f"Loading environment from {env_path}")
        with open(env_path, 'r') as f:
            for line in f:
                if line.strip() and not line.startswith('#'):
                    key, value = line.strip().split('=', 1)
                    if key not in os.environ:
                        os.environ[key] = value.strip('"').strip("'")

def run_command(cmd, cwd=None, env=None, check=True):
    print(f"Running: {cmd}")
    ret = subprocess.call(cmd, shell=True, cwd=cwd, env=env)
    if check and ret != 0:
        print(f"Command failed: {cmd}")
        sys.exit(ret)
    return ret

def save_gcda(filename, hex_data, test_dir):
    print(f"Saving {filename}...")
    safe_name = filename.replace("/", "_").replace("\\", "_")
    if safe_name.startswith("_"): safe_name = safe_name[1:]
    
    # Only save coverage files for our tests/source
    if "test_" not in filename and "test_common" not in filename:
        return

    out_path = os.path.join(test_dir, "coverage", safe_name + ".gcda")
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    
    with open(out_path, 'wb') as f:
        f.write(binascii.unhexlify(hex_data))

def parse_output_for_coverage(output, test_dir):
    print("Parsing output for GCOV data...")
    if "=== GCOV DUMP START ===" not in output:
        print("Error: GCOV dump start marker not found.")
        # Print tail/grep to hint what happened
        return False
        
    lines = output.splitlines()
    parsing = False
    current_file = None
    hex_data = ""
    
    for line in lines:
        line = line.strip()
        if line == "=== GCOV DUMP START ===":
            parsing = True
            continue
        if line == "=== GCOV DUMP END ===":
            parsing = False
            continue
            
        if parsing:
            if line.startswith("FILE: "):
                if current_file and hex_data:
                    save_gcda(current_file, hex_data, test_dir)
                current_file = line[6:].strip()
                hex_data = ""
            elif line.startswith("DATA: "):
                hex_data += line[6:].strip()
                
    if current_file and hex_data:
        save_gcda(current_file, hex_data, test_dir)
    return True

def run_qemu(test_dir):
    print(" Preparing QEMU execution...")
    # 1. Merge binaries
    # We assume 'idf.py build' has run.
    # We need to create a single binary for QEMU (esp32 machine usually takes raw flash image or elf?)
    # QEMU esp32 machine supports loading ELF directly or flash image.
    # Flash image is safer for partition table etc.
    
    build_dir = os.path.join(test_dir, "build")
    merged_bin = os.path.join(build_dir, "merged-qemu.bin")
    
    # Use esptool to merge
    # We can try to use the python env's esptool
    cmd_merge = f"esptool.py --chip esp32 merge_bin -o {merged_bin} --fill-flash-size 4MB @flash_args"
    run_command(cmd_merge, cwd=build_dir)
    
    print(" Starting QEMU...")
    # 2. Run QEMU
    # -nographic to capture stdio
    # -machine esp32
    # -drive file=...,if=mtd,format=raw
    # -serial mon:stdio
    
    cmd_qemu = f"qemu-system-xtensa -nographic -machine esp32 -drive file={merged_bin},if=mtd,format=raw -serial mon:stdio"
    
    # We run with timeout because QEMU might loop if test doesn't exit (though user added loop exit)
    # But QEMU itself doesn't exit when CPU halts usually, unless semihosting exit?
    # We rely on timeout or detecting "GCOV dump complete" output?
    # Let's use timeout for safety.
    
    print(f"Running: {cmd_qemu}")
    try:
        # Capture output. 
        # Using a timeout to ensure we don't hang forever.
        result = subprocess.run(cmd_qemu, shell=True, cwd=build_dir, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=180)
        output = result.stdout
    except subprocess.TimeoutExpired as e:
        print("QEMU timeout (expected if it doesn't auto-quit).")
        output = e.stdout.decode('utf-8') if e.stdout else ""
        
    return output

def run_wokwi(test_dir):
    print(" Starting Wokwi...")
    wokwi_cmd = f"wokwi-cli --timeout 120000" 
    
    try:
        result = subprocess.run(wokwi_cmd, shell=True, cwd=test_dir, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=140)
        output = result.stdout
    except subprocess.TimeoutExpired as e:
        print("Wokwi timed out!")
        output = e.stdout.decode('utf-8') if e.stdout else ""
    return output

def main():
    parser = argparse.ArgumentParser(description="Run coverage for Unity HW Tests")
    parser.add_argument("--wokwi", action="store_true", help="Use Wokwi simulator instead of QEMU")
    parser.add_argument("--skip-build", action="store_true", help="Skip the build step")
    args = parser.parse_args()

    root_dir = os.getcwd()
    test_dir = os.path.join(root_dir, PROJECT_DIR)
    load_env(root_dir)
    
    # 1. Build
    if not args.skip_build:
        print("Building project...")
        run_command("idf.py -D SDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.coverage' build", cwd=test_dir)
    else:
        print("Skipping build...")

    # 2. Run Simulation
    output = ""
    if args.wokwi:
        output = run_wokwi(test_dir)
    else:
        output = run_qemu(test_dir)
    
    # 3. Parse Output
    if not parse_output_for_coverage(output, test_dir):
        print("Failed to extract coverage data.")
        # Debug: print last few lines
        print("Output tail:")
        print("\n".join(output.splitlines()[-20:]))
        sys.exit(1)

    # 4. Organize files (Copy .gcda to build dir where .gcno are)
    print("Organizing coverage files...")
    gcda_files = glob.glob(os.path.join(test_dir, "coverage", "*.gcda"))
    
    for gcda in gcda_files:
        name = os.path.basename(gcda)
        simple_name = None
        if "test_entry_gate_hw.cpp" in name: simple_name = "test_entry_gate_hw.cpp"
        elif "test_exit_gate_hw.cpp" in name: simple_name = "test_exit_gate_hw.cpp"
        elif "test_common.cpp" in name: simple_name = "test_common.cpp"
        
        if simple_name:
             found = False
             for root, dirs, files in os.walk(os.path.join(test_dir, "build")):
                 for f in files:
                     if f.endswith(".gcno") and simple_name in f:
                         target_path = os.path.join(root, simple_name + ".gcda")
                         print(f"copy {gcda} -> {target_path}")
                         shutil.copy(gcda, target_path)
                         found = True
                         break
                 if found: break

    # 5. Generate Report
    print("Generating HTML report...")
    run_command("mkdir -p coverage_report", cwd=test_dir)
    run_command("gcovr --gcov-executable xtensa-esp32-elf-gcov --html --html-details -o coverage_report/index.html -r .", cwd=test_dir)
    print(f"Done! Report: {os.path.join(test_dir, 'coverage_report/index.html')}")

if __name__ == "__main__":
    main()

