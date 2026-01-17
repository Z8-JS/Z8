import re
import os

lib_path = 'libs/v8_monolith.lib'

if not os.path.exists(lib_path):
    print(f"Warning: {lib_path} not found. Please place your V8 monolith library there.")
    # Create empty shims if lib not found just to allow script to run
    symbols = []
else:
    with open(lib_path, 'rb') as f:
        content = f.read().decode('ascii', errors='ignore')
    symbols = set(re.findall(r'temporal_rs_[a-zA-Z0-9_]+', content))

with open('src/temporal_shims.cpp', 'w') as f:
    f.write('extern "C" {\n')
    for sym in sorted(symbols):
        f.write(f'  void {sym}() {{}}\n')
    f.write('}\n')

print(f"Extracted {len(symbols)} symbols into src/temporal_shims.cpp")
