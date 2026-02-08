#!/usr/bin/env python3
"""
Z8 Coding Style Checker
Automatically validates C++ code against Z8 coding standards.

Usage:
    python check_style.py [path]
    
    If no path is provided, checks all C++ files in src/
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Tuple

class StyleChecker:
    def __init__(self):
        self.errors = []
        self.warnings = []
        self.files_checked = 0
        
    def check_file(self, filepath: Path) -> bool:
        """Check a single file for style violations."""
        if not filepath.suffix in ['.cpp', '.h']:
            return True
            
        # Skip external files
        skip_patterns = ['temporal_shims.cpp', 'v8', 'libs/']
        if any(pattern in str(filepath) for pattern in skip_patterns):
            return True
            
        self.files_checked += 1
        content = filepath.read_text(encoding='utf-8', errors='ignore')
        lines = content.split('\n')
        
        has_errors = False
        
        # Check for PascalCase methods (should be camelCase)
        for i, line in enumerate(lines, 1):
            # Check for method definitions with PascalCase
            if re.search(r'void\s+\w+::[A-Z]\w+\s*\(', line):
                method_match = re.search(r'void\s+\w+::([A-Z]\w+)\s*\(', line)
                if method_match:
                    method_name = method_match.group(1)
                    # Allow constructors and destructors
                    if not method_name.startswith('~'):
                        self.errors.append(
                            f"{filepath}:{i}: Method '{method_name}' should use camelCase"
                        )
                        has_errors = True
            
            # Check for member variables without m_ prefix in class definitions
            if re.search(r'^\s+(int|bool|std::|v8::)\w+\s+[^m_]\w+;', line):
                # This is a simple heuristic - might need refinement
                if 'class ' not in lines[max(0, i-10):i]:  # Not in class definition
                    continue
                var_match = re.search(r'^\s+(?:int|bool|std::|v8::)\w+\s+(\w+);', line)
                if var_match and not var_match.group(1).startswith('m_'):
                    self.warnings.append(
                        f"{filepath}:{i}: Member variable '{var_match.group(1)}' should use m_ prefix"
                    )
            
            # Check for int instead of int32_t
            if re.search(r'\bint\s+\w+\s*[=;]', line) and 'int32_t' not in line:
                # Skip if it's a comment or string
                if '//' not in line[:line.find('int')] and '/*' not in line:
                    self.warnings.append(
                        f"{filepath}:{i}: Use 'int32_t' instead of 'int' for integer types"
                    )
        
        return not has_errors
    
    def check_directory(self, directory: Path) -> bool:
        """Recursively check all C++ files in directory."""
        all_good = True
        
        for filepath in directory.rglob('*.cpp'):
            if not self.check_file(filepath):
                all_good = False
                
        for filepath in directory.rglob('*.h'):
            if not self.check_file(filepath):
                all_good = False
                
        return all_good
    
    def print_report(self):
        """Print the style check report."""
        print(f"\n{'='*70}")
        print(f"Z8 Coding Style Check Report")
        print(f"{'='*70}")
        print(f"Files checked: {self.files_checked}")
        print(f"Errors: {len(self.errors)}")
        print(f"Warnings: {len(self.warnings)}")
        print(f"{'='*70}\n")
        
        if self.errors:
            print("❌ ERRORS (Must Fix):")
            for error in self.errors:
                print(f"  {error}")
            print()
        
        if self.warnings:
            print("⚠️  WARNINGS (Should Fix):")
            for warning in self.warnings[:20]:  # Limit to first 20
                print(f"  {warning}")
            if len(self.warnings) > 20:
                print(f"  ... and {len(self.warnings) - 20} more warnings")
            print()
        
        if not self.errors and not self.warnings:
            print("✅ All checks passed! Code follows Z8 coding standards.")
        
        return len(self.errors) == 0

def main():
    checker = StyleChecker()
    
    # Determine what to check
    if len(sys.argv) > 1:
        # Get the absolute path and validate it's within project
        try:
            target_path = os.path.abspath(sys.argv[1])
            project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
            
            # Security: Prevent path traversal outside project directory
            if os.path.commonpath([project_root, target_path]) != project_root:
                 print(f"Error: Path '{sys.argv[1]}' is outside the project directory")
                 sys.exit(1)
            
            target = Path(target_path)
        except (OSError, RuntimeError) as e:
            print(f"Error: Invalid path '{sys.argv[1]}': {e}")
            sys.exit(1)
    else:
        # Default to src directory
        target = Path(__file__).parent.parent / 'src'
    
    if not target.exists():
        print(f"Error: Path '{target}' does not exist")
        sys.exit(1)
    
    print(f"🔍 Checking coding style in: {target}")
    print(f"{'='*70}\n")
    
    if target.is_file():
        success = checker.check_file(target)
    else:
        success = checker.check_directory(target)
    
    success = checker.print_report()
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
