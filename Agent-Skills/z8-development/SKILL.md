---
name: z8-development
description: Guidelines and instructions for developing the Z8 runtime. Includes coding standards, build processes, and testing workflows.
license: Apache-2.0
metadata:
  author: Zane V8 Authors
  version: "1.0.0"
---

# Z8 Development Skill

This skill provides instructions for maintaining and extending the Z8 JavaScript runtime.

## Coding Standards

All C++ code in the Z8 project should adhere to the following standards:

- **Methods**: Use `camelCase` (e.g., `readFile`, `setTimeout`).
- **Member Variables**: Prefix with `m_` (e.g., `m_count`, `m_isRunning`).
- **Pointers**: Prefix with `p_` (e.g., `p_isolate`, `p_context`).
- **Static Variables**: Prefix with `s_` (e.g., `s_instance`).
- **Integer Types**: Use `int32_t` instead of `int`.
- **Reference Docs**: Follow the guidelines in `docs/CODING_STYLE_en.md` and `docs/CODING_STYLE_vi.md`.

## Build Process

The project is built using a PowerShell script:

- Run `.\build.ps1` from the `Z8-app` directory.
- For a clean build, you may need to remove `.obj` files.

## Testing & Validation

Before committing changes, ensure the following checks pass:

- **Style Check**: Run `python tools/check_style.py`.
- **Benchmark**: Run `.\z8.exe ..\TEST\benchmark_fs.js` to verify performance consistency.
- **Git Hooks**: Ensure `python tools/install_hooks.py` has been run to enable pre-commit checks.

## Repository Structure

- `src/`: Core C++ source files.
- `tools/`: Development and build support tools.
- `docs/`: Documentation and coding style guides.
- `Agent-Skills/`: AI agent capability definitions (this directory).
