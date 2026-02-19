import os
import sys
import urllib.request
import zipfile
import tarfile
import shutil
import time

# Thư mục chứa các thư viện phụ thuộc
DEPS_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "deps")

# Danh sách các thư viện cần tải (Ví dụ bản pre-built cho Windows x64)
# Lưu ý: Các URL này là ví dụ, bạn nên cập nhật bản ổn định nhất
DEPENDENCIES = {
    "openssl": {
        "url": "https://github.com/openssl/openssl/releases/download/openssl-3.6.1/openssl-3.6.1.tar.gz",
        "folder": "openssl",
        "is_source": True
    },
    "zlib": {
        "url": "https://github.com/madler/zlib/releases/download/v1.3.2/zlib132.zip",
        "folder": "zlib",
        "is_source": True
    },
    "uWebSockets": {
        "url": "https://github.com/uNetworking/uWebSockets/archive/refs/tags/v20.74.0.zip",
        "folder": "uWebSockets",
        "is_source": True
    },
    "uSockets": {
        "url": "https://github.com/uNetworking/uSockets/archive/refs/tags/v0.8.8.zip",
        "folder": "uSockets",
        "is_source": True
    }
}

def download_file(url, target_path):
    print(f"📥 Đang tải: {url}...")
    try:
        urllib.request.urlretrieve(url, target_path)
        print(f"✅ Đã tải xong: {target_path}")
    except Exception as e:
        print(f"❌ Lỗi khi tải: {e}")
        return False
    return True

def extract_file(file_path, extract_to):
    print(f"📦 Đang giải nén: {file_path}...")
    try:
        if file_path.endswith('.zip'):
            with zipfile.ZipFile(file_path, 'r') as zip_ref:
                zip_ref.extractall(extract_to)
        elif file_path.endswith(('.tar.gz', '.tgz')):
            with tarfile.open(file_path, 'r:gz') as tar_ref:
                tar_ref.extractall(extract_to)
        else:
            print(f"❌ Định dạng file không hỗ trợ: {file_path}")
            return False
        print(f"✅ Giải nén xong vào: {extract_to}")
    except Exception as e:
        print(f"❌ Lỗi giải nén: {e}")
        return False
    return True

def setup_dependencies():
    if not os.path.exists(DEPS_DIR):
        os.makedirs(DEPS_DIR)
        print(f"📂 Đã tạo thư mục deps tại: {DEPS_DIR}")

    for name, info in DEPENDENCIES.items():
        target_folder = os.path.join(DEPS_DIR, info["folder"])
        
        # Nếu thư mục đã tồn tại, bỏ qua (hoặc bạn có thể thêm logic force update)
        if os.path.exists(target_folder):
            print(f"⏭️  Thư viện {name} đã tồn tại, bỏ qua.")
            continue

        ext = ".tar.gz" if info["url"].endswith(".tar.gz") else ".zip"
        temp_file = os.path.join(DEPS_DIR, f"{name}{ext}")
        
        if download_file(info["url"], temp_file):
            if extract_file(temp_file, DEPS_DIR):
                # Xử lý trường hợp archive chứa thư mục con lồng nhau
                if info.get("is_source"):
                    # Tìm thư mục vừa giải nén (thường có tên dạng uWebSockets-20.74.0 hoặc openssl-3.6.1)
                    # Chúng ta bỏ qua các folder đã tồn tại đúng tên
                    extracted_dirs = [d for d in os.listdir(DEPS_DIR) 
                                    if os.path.isdir(os.path.join(DEPS_DIR, d)) 
                                    and d.lower().startswith(info["folder"].lower())
                                    and d.lower() != info["folder"].lower()]
                    
                    if extracted_dirs:
                        source_dir = os.path.join(DEPS_DIR, extracted_dirs[0])
                        if source_dir != target_folder:
                            print(f"🚚 Di chuyển {extracted_dirs[0]} -> {info['folder']}...")
                            if os.path.exists(target_folder):
                                shutil.rmtree(target_folder)
                            
                            # Thêm thời gian trễ nhỏ cho Windows để thả lỏng file lock
                            if os.name == 'nt':
                                time.sleep(1)
                                
                            success = False
                            for i in range(5):
                                try:
                                    if os.path.exists(target_folder):
                                        shutil.rmtree(target_folder)
                                    shutil.move(source_dir, target_folder)
                                    success = True
                                    break
                                except Exception as e:
                                    print(f"⚠️ Thử lần {i+1} - Không thể di chuyển thư mục: {e}")
                                    time.sleep(1)
                            
                            if not success:
                                print(f"❌ Thất bại hoàn toàn khi di chuyển {name}. Vui lòng di chuyển thủ công.")
                
                if os.path.exists(temp_file):
                    os.remove(temp_file)
                print(f"🎉 Hoàn tất thiết lập {name}!")

if __name__ == "__main__":
    print("🚀 Bắt đầu thiết lập các thư viện phụ thuộc cho Z8...")
    setup_dependencies()
    print("\n✨ Tất cả thư viện đã sẵn sàng trong thư mục /deps")
