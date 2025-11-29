import subprocess
import datetime
import sys
import os
from collections import Counter

# Configuration
FILE_PATH = 'webgpu-cts/expectations.txt'
START_DATE = datetime.datetime(2025, 3, 29)
END_DATE = datetime.datetime(2025, 11, 28) # Today
# END_DATE = datetime.datetime.now()

def run_git_command(args):
    result = subprocess.run(args, capture_output=True, text=True, cwd=os.getcwd())
    if result.returncode != 0:
        # It's possible the file didn't exist way back then or other git errors
        return None
    return result.stdout.strip()

def get_file_content_at_commit(commit_hash, file_path):
    return run_git_command(['git', 'show', f'{commit_hash}:{file_path}'])

def get_commit_before_date(date):
    date_str = date.strftime('%Y-%m-%d')
    # Find the last commit on or before this date
    cmd = ['git', 'rev-list', '-n', '1', f'--before={date_str}', 'HEAD']
    return run_git_command(cmd)

def parse_expectations(content):
    if content is None:
        return set()
    lines = set()
    for line in content.splitlines():
        line = line.strip()
        if line and not line.startswith('#') and 'webgpu:shader' in line:
            lines.add(line)
    return lines

def main():
    current_date = START_DATE
    
    # Map line_content -> generation_id
    active_lines = {}
    
    generation_id = 0
    
    print(f"{'Week':<12} {'Date':<12} {'Total':<6} {'Generations (Gen:Count)...'}")
    print("-" * 80)

    while current_date <= END_DATE:
        commit = get_commit_before_date(current_date)
        
        if not commit:
            print(f"No commit found before {current_date.strftime('%Y-%m-%d')}")
            current_date += datetime.timedelta(weeks=1)
            generation_id += 1
            continue

        content = get_file_content_at_commit(commit, FILE_PATH)
        current_lines = parse_expectations(content)
        
        next_active_lines = {}
        
        # Identify lines and assign generations
        for line in current_lines:
            if line in active_lines:
                # Keep existing generation
                next_active_lines[line] = active_lines[line]
            else:
                # New line, assign current generation
                next_active_lines[line] = generation_id
        
        active_lines = next_active_lines
        
        # Calculate stats
        gen_counts = Counter(active_lines.values())
        
        # Format output
        # We want to show counts for all generations up to current
        # But that might be too wide. Let's print a summary or CSV format.
        # The user asked for "listing of each week containing the counts for each generation"
        
        # Let's construct a list of counts ordered by generation
        counts_list = []
        for g in range(generation_id + 1):
            counts_list.append(str(gen_counts.get(g, 0)))
        
        # To make it readable, maybe just print the list
        date_str = current_date.strftime('%Y-%m-%d')
        total = len(active_lines)
        
        # Output format: Week Date Total [Gen0, Gen1, Gen2, ...]
        # This might get very long (80 numbers).
        # I'll print it as a CSV line essentially.
        row_str = f"Week {generation_id:<4} {date_str} {total:<6} {','.join(counts_list)}"
        print(row_str)
        
        # Advance time
        current_date += datetime.timedelta(weeks=1)
        generation_id += 1

if __name__ == '__main__':
    main()
