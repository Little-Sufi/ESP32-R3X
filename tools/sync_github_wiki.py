import subprocess
import os
import sys
import tempfile
import shutil
import re

def get_token():
    remote = subprocess.check_output(['git', 'remote', 'get-url', 'origin'], cwd=r'D:\ESP32-R3X', text=True).strip()
    match = re.search(r'ghp_[A-Za-z0-9]+', remote)
    if match:
        return match.group(0)
    raise RuntimeError("No token found in remote URL")

def sync_wiki(repo_name, wiki_dir):
    token = get_token()
    wiki_url = f"https://Little-Sufi:{token}@github.com/Little-Sufi/{repo_name}.wiki.git"
    print(f"\n[*] Checking wiki repository for {repo_name} ({wiki_url.replace(token, 'TOKEN')})...")
    
    with tempfile.TemporaryDirectory() as td:
        # Check if remote exists
        check = subprocess.run(['git', 'ls-remote', wiki_url], capture_output=True, text=True)
        if check.returncode != 0:
            print(f"[-] {repo_name}.wiki.git is not yet initialized on GitHub.")
            print(f"[!] Please open https://github.com/Little-Sufi/{repo_name}/wiki in your browser and click 'Create the first page' -> 'Save Page'.")
            return False

        print(f"[+] Remote {repo_name}.wiki.git is initialized! Cloning...")
        clone_res = subprocess.run(['git', 'clone', wiki_url, td], capture_output=True, text=True)
        if clone_res.returncode != 0:
            print(f"[-] Clone failed: {clone_res.stderr}")
            return False

        # Copy all files from wiki_dir to td
        for f in os.listdir(wiki_dir):
            if f.endswith('.md'):
                src = os.path.join(wiki_dir, f)
                dst = os.path.join(td, f)
                shutil.copy2(src, dst)
                print(f"    Copied: {f}")

        # Git add, commit, push
        subprocess.run(['git', 'config', 'user.name', 'Little-Sufi'], cwd=td, check=True)
        subprocess.run(['git', 'config', 'user.email', 'littlesufi3@gmail.com'], cwd=td, check=True)
        subprocess.run(['git', 'add', '-A'], cwd=td, check=True)
        
        status = subprocess.run(['git', 'status', '--porcelain'], cwd=td, capture_output=True, text=True)
        if not status.stdout.strip():
            print(f"[+] Wiki for {repo_name} is already up to date!")
            return True

        subprocess.run(['git', 'commit', '-m', f"docs(wiki): update {repo_name} official wiki pages"], cwd=td, check=True)
        push_res = subprocess.run(['git', 'push', 'origin', 'master'], cwd=td, capture_output=True, text=True)
        if push_res.returncode == 0:
            print(f"[+] SUCCESS: {repo_name} wiki updated and pushed to GitHub!")
            return True
        else:
            # Try pushing to main
            push_res2 = subprocess.run(['git', 'push', 'origin', 'main'], cwd=td, capture_output=True, text=True)
            if push_res2.returncode == 0:
                print(f"[+] SUCCESS: {repo_name} wiki updated and pushed to GitHub (main)!")
                return True
            print(f"[-] Push failed: {push_res.stderr} / {push_res2.stderr}")
            return False

if __name__ == '__main__':
    print("=== GitHub Wiki Synchronizer ===")
    r3x_ok = sync_wiki("ESP32-R3X", r"D:\ESP32-R3X\wiki")
    victor_ok = sync_wiki("victor", r"E:\VICTOR\victor\wiki")
