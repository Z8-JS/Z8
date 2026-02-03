import subprocess
import os
import sys
import time

def run_git(args, cwd):
    result = subprocess.run(['git'] + args, cwd=cwd, capture_output=True, text=True, encoding='utf-8')
    if result.returncode != 0:
        print(f"⚠️ Git command failed: git {' '.join(args)}\nStandard Output:\n{result.stdout}\nError Output:\n{result.stderr}")
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

    branch = sys.argv[1] if len(sys.argv) > 1 else "main"
    message = sys.argv[2] if len(sys.argv) > 2 else "Final sync from Gravity Bot"
    
    root_dir = "d:/Z8"
    
    print(f"🚀 [Gravity Bot] Đang xử lý trên nhánh {branch}...")

    run_git(['config', 'user.name', 'Gravity Bot'], root_dir)
    run_git(['config', 'user.email', 'orbbrowser@gmail.com'], root_dir)
    
    # Đảm bảo chúng ta đang ở nhánh đúng
    current_branch = run_git(['rev-parse', '--abbrev-ref', 'HEAD'], root_dir)
    if current_branch != branch:
        print(f"🔄 Định vị lại nhánh {branch} vào HEAD hiện tại (fix detached HEAD)...")
        # Force create/reset branch main to current HEAD
        if run_git(['checkout', '-B', branch], root_dir) is None:
             print("❌ Không thể chuyển nhánh.")
             return

    # 1. Add Files
    print(f"➕ Đang thêm file...")
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

    # 2. Commit (Commit trước khi pull để bảo toàn thay đổi local)
    print(f"💾 Đang commit...")
    commit_res = run_git(['commit', '-m', message], root_dir)
    if commit_res is None:
        print("ℹ️ Không có thay đổi nào cần commit hoặc commit lỗi.")
    
    remote_url = f"https://{token}@github.com/{owner}/{repo_name}.git"

    # 3. Pull (để merge changes từ remote)
    print(f"📥 Đang kéo code mới nhất từ remote (Allow unrelated histories)...")
    # Use remote_url instead of 'origin' to ensure we pull from the right repo
    # Allow unrelated histories to fix sync issues if repos diverged/orphaned
    run_git(['pull', remote_url, branch, '--allow-unrelated-histories', '--no-edit'], root_dir)

    # 4. Push
    print(f"🌍 Đang đẩy code...")
    result = subprocess.run(['git', 'push', '--set-upstream', remote_url, branch], cwd=root_dir, capture_output=True, text=True)
    
    if result.returncode == 0:
        print(f"✅ THÀNH CÔNG! Đã đẩy lên nhánh {branch}.")
    else:
        print(f"❌ Thất bại:\n{result.stderr.replace(token, '***')}")

if __name__ == "__main__":
    main()
