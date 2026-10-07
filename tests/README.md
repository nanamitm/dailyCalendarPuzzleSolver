# Regression tests

Run from the repository root:

```sh
python -B -m unittest discover -s py -p test_regressions.py
python -B tests/test_batch_script.py
node tests/web_regressions.mjs
cmake -S tests -B build-tests -DCMAKE_BUILD_TYPE=Debug
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

Native tests require Qt 6 Widgets and a C++17 compiler. Set
`CMAKE_PREFIX_PATH` to the Qt kit if CMake cannot discover it. The drawing
test uses Qt's offscreen platform, so a display is not required. The batch
script test requires Bash (Git for Windows is supported) and uses a stub
solver to check all 2,604 date invocations without a full puzzle search.

The tests cover Python source syntax, front/back symmetry, solver reuse,
incomplete piece sets, malformed JSON, bundled preset compatibility,
out-of-bounds coordinates, eight-cell Maker pieces, drawing more than ten
pieces, default solution/try counts, cancellation before thread startup,
shutdown during a search, cancellation while paused or generating shapes,
and monthly JSON export. Android backend shutdown is tested on the host;
APK packaging and device interaction still require Android testing.

For GCC bounds and integer-overflow checks, configure a separate build with:

```sh
cmake -S tests -B build-tests-checked -DCMAKE_BUILD_TYPE=Debug \
  '-DCMAKE_CXX_FLAGS=-fsanitize=bounds,signed-integer-overflow -fsanitize-undefined-trap-on-error'
```
