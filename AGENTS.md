# Repository Guidelines

## Project Structure & Module Organization
- `src/`: C++ implementation (device clients, DB layer, check manager, entry `main.cpp`).
- `include/`: headers mirroring `src/` namespaces (`client`, `device`, `db`, `utils`).
- `qml/`: QML UI entrypoints (`Main.qml`, `page/Home.qml`) and reusable components under `components/` with shared styles in `style/`.
- `config/base.yaml`: runtime configuration for DB, RabbitMQ, Redis, and device settings.
- `assets/`: packaged fonts; `doc/` holds architecture and DB design notes; `build/` is CMake/Ninja output.

## Build, Test, and Development Commands
- `just cmake`: configure the project (`build/`) with Ninja; honors `QT_DIR`, `DCMAKE_TOOLCHAIN_FILE` (vcpkg), and `BUILD_TYPE` (default `Release`).
- `just build` (alias `just b`): compile the Qt/QML app via Ninja into `build/`.
- `just run` (alias `just r`): run the GUI binary with `LD_LIBRARY_PATH` set for Qt 6.8.3.
- `just clean`: remove the `build/` directory.
- If `just` is unavailable, run `cmake -S . -B build -G Ninja ...` then `ninja -C build`.

## Coding Style & Naming Conventions
- Prefer 2-space indentation as used in current sources; keep braces on new lines for namespaces/classes, same line for functions.
- Use PascalCase for functions/classes (`InitClient`, `DeviceManager`); namespaces remain lowercase (`client`, `device`).
- Keep headers and sources paired (`include/<module>/<name>.h` <-> `src/<module>/<name>.cpp`).
- Run `clangd` tooling for completions and diagnostics; align includes by dependency order (STD/3rd-party/project).

## Testing Guidelines
- No automated test suite is present yet. Before changes, ensure `just build` succeeds and `just run` exercises key flows (device loading, message subscription, UI rendering).
- Add new tests alongside future modules (e.g., GoogleTest for C++ helpers) and mirror namespaces in `tests/` when introduced.

## Commit & Pull Request Guidelines
- Write imperative, scoped commit messages; prefer Conventional Commit prefixes (`feat:`, `fix:`, `chore:`, `docs:`) for clarity.
- PRs should summarize scope, list testing performed (`just build` / manual UI checks), and link related issues. Include screenshots or logs for UI or integration-visible changes.
- Keep config edits (`config/base.yaml`, `.env`) documented in the PR and avoid committing secrets; use placeholders when sharing configs.

## Security & Configuration Tips
- Never commit real credentials; `.env` should stay local. Keep `config/base.yaml` to non-sensitive defaults and document local overrides in the PR.
- When enabling new services (RabbitMQ/Redis/MySQL), verify connectivity on startup and log failures using `glog` to aid operators.
