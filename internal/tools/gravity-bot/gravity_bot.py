import subprocess
import os
import sys
import time

def run_git(args, cwd):
    result = subprocess.run(['git'] + args, cwd=cwd, capture_output=True, text=True, encoding='utf-8')
    if result.returncode != 0:
        return None
    return result.stdout.strip()

def main():
    env_path = os.path.join(os.path.dirname(__file__), '.env')
    env = {}
    if os.path.exists(env_path):
        with open(env_path, 'r', encoding='utf-8') as f:
            for line in f:
                if '=' in line:
                    k, v = line.split('=', 1)
                    env[k.strip()] = v.strip()

    token = env.get('GITHUB_TOKEN')
    owner = env.get('REPO_OWNER', 'Z8-JS')
    repo_name = env.get('REPO_NAME', 'Z8')

    if not token or len(token) < 10:
        print("❌ Lỗi: Không tìm thấy GITHUB_TOKEN hợp lệ trong .env")
        return

    branch = sys.argv[1] if len(sys.argv) > 1 else "feat/fs-v8-final"
    message = sys.argv[2] if len(sys.argv) > 2 else "Final sync from Gravity Bot"
    
    root_dir = "d:/Z8"
    temp_name = f"sync_{int(time.time())}"
    
    print(f"🚀 [Gravity Bot] Đang đẩy code lên {branch}...")

    run_git(['config', 'user.name', 'Gravity Bot'], root_dir)
    run_git(['config', 'user.email', 'gravity-bot@z8.internal'], root_dir)
    
    # Tạo nhánh orphan hoàn toàn mới để tránh file cũ
    run_git(['checkout', '--orphan', temp_name], root_dir)
    run_git(['reset'], root_dir) # Clear index

    # Chỉ add mã nguồn
    essential_items = [
        'Z8-app/src/', 
        'Z8-app/build.ps1', 
        'Z8-app/.gitignore',
        '.gitignore'
    ]
    for f in essential_items:
        if os.path.exists(os.path.join(root_dir, f)):
            run_git(['add', f], root_dir)
            
    run_git(['add', 'internal/tools/gravity-bot/gravity-bot.mjs'], root_dir)
    run_git(['add', 'internal/tools/gravity-bot/gravity_bot.py'], root_dir)

    run_git(['commit', '-m', message], root_dir)

    remote_url = f"https://{token}@github.com/{owner}/{repo_name}.get"
    # Sửa lỗi .get thành .git
    remote_url = remote_url.replace('.get', '.git')
    
    print(f"🌍 Đang đẩy code...")
    result = subprocess.run(['git', 'push', '--force', remote_url, f'HEAD:refs/heads/{branch}'], cwd=root_dir, capture_output=True, text=True)
    
    if result.returncode == 0:
        print(f"✅ THÀNH CÔNG! Đã đẩy lên nhánh {branch}.")
    else:
        print(f"❌ Thất bại: {result.stderr.replace(token, '***')}")
    
    # Cleanup
    run_git(['checkout', 'main'], root_dir)
    run_git(['branch', '-D', temp_name], root_dir)

if __name__ == "__main__":
    main()
