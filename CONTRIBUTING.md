# Contributing to wtop

Thanks for your interest in improving wtop! Bug reports, feature ideas and pull
requests are all welcome.

## Ground rules

- **Linux only.** wtop reads `/proc` and `/sys`; please don't add code paths
  for other operating systems.
- **Standard library only.** The `wtop` program depends on nothing but the
  C++20 standard library and POSIX/Linux system interfaces. Don't add
  third-party runtime libraries (no ncurses, fmt, Boost, ...). GoogleTest is
  allowed for tests only.
- **Keep it Task Manager-like.** New screens and fields should follow the
  layout and wording of the Windows Task Manager *Performance* page where a
  Linux equivalent exists.

## Development setup

You need a C++20 compiler (GCC 13+ or Clang 17+), CMake 3.20+ and, for the
tests, network access the first time you configure (GoogleTest is downloaded
unless it is already installed).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/wtop
```

A sanitizer build catches most memory and undefined-behavior bugs:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build build-asan -j
ctest --test-dir build-asan --output-on-failure
```

`./build/wtop --snapshot 120x40` prints every screen as plain text, which is
the quickest way to check a layout change or attach output to a bug report.

## Coding style

wtop follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html):

- `.h` / `.cc` files named in `snake_case`, header guards
  `WTOP_SRC_<PATH>_<FILE>_H_`, everything inside `namespace wtop`.
- `PascalCase` types and functions, `snake_case` variables, trailing
  underscore for members (`history_`), `kConstant` for constants.
- Format with the bundled `.clang-format` before committing:

  ```bash
  clang-format -i $(find src tests -name '*.h' -o -name '*.cc')
  ```

- Build must stay free of warnings under `-Wall -Wextra -Wpedantic`.

## Architecture in one minute

- `src/collectors/` read the system. Parsing lives in pure functions in
  `proc_parsers.cc` that take file contents as a string, so they can be unit
  tested with fixture text.
- `src/ui/views/` turn collector data into a `PaneContent` description; views
  never touch the system.
- `src/ui/` draws into a `ScreenBuffer`, which is diffed against the previous
  frame so only changed cells are written to the terminal.

To add a new resource screen: write a collector (plus parser tests), a view
that fills `PaneContent` and `SidebarEntry`, and register it in `App::App`.

## Tests

Tests use [GoogleTest](https://github.com/google/googletest) and live in
`tests/`. Please add or update tests for:

- every new or changed parser (use real `/proc` / `/sys` text as fixtures),
- formatting helpers,
- key decoding.

## Branches

- `main` holds released code only and is the default branch, so the
  `.../HEAD/install.sh` URL always serves the released installer.
- `develop` collects finished work; open pull requests against `develop`.
- `feature/<name>` for each change, `release/<version>` to prepare a release.
- Versions are **tags** (`v0.1.0`), never branch names.

Every push and pull request to `main` or `develop` runs
[CI](.github/workflows/ci.yml): builds with GCC 13, GCC 14 and Clang 18, the
unit tests, an ASan/UBSan build, clang-format, ShellCheck and an installer
test.

## Releasing

wtop uses [Semantic Versioning](https://semver.org). The version lives only
in `project(wtop VERSION x.y.z)` in `CMakeLists.txt`.

1. On `develop`, create `release/x.y.z`.
2. Set `VERSION x.y.z` in `CMakeLists.txt` and update the date in
   `docs/wtop.1.in`.
3. In `CHANGELOG.md`, move the *Unreleased* entries under
   `## [x.y.z] - YYYY-MM-DD` and update the links at the bottom.
4. Merge `release/x.y.z` into `main`, then tag and push the tag:

   ```bash
   git tag -a vx.y.z -m "wtop x.y.z"
   git push origin vx.y.z
   ```

5. The [release workflow](.github/workflows/release.yml) checks that the tag
   matches `CMakeLists.txt` and `CHANGELOG.md`, builds static binaries for
   x86_64 and aarch64 in Alpine (running the tests there), checks they start
   on Debian 11, Ubuntu 20.04 and AlmaLinux 8, and publishes the GitHub
   Release with `SHA256SUMS`, build provenance attestations and the changelog
   as release notes. `install.sh` picks it up immediately.
6. Merge `main` back into `develop`.

To rebuild the assets of an existing tag, run the *Release* workflow manually
from the Actions tab and enter the tag.

## Pull requests

1. Fork and create a topic branch.
2. Keep changes focused; one feature or fix per pull request.
3. Make sure `ctest` passes, the build has no warnings and the code is
   formatted.
4. Update `README.md` and `docs/wtop.1.in` when you change options, keys or
   visible behavior.
5. Describe what changed and how you tested it; a `--snapshot` excerpt helps
   for UI changes.

## Reporting bugs

Please include:

- `wtop --version` output and your Linux distribution / kernel version,
- the terminal emulator you use,
- the output of `wtop --snapshot` (remove anything you consider private,
  such as IP or MAC addresses),
- what you expected to see.

By contributing you agree that your contributions are licensed under the
[MIT License](LICENSE.md).
