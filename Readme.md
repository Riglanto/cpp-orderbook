# Run

CMake tool for VScode

Via command line:

`cmake -DCMAKE_BUILD_TYPE=Release -B build && cd build/ && cmake --build . && ./sq && cd -`

No external libs needed.

# Notes & tricks

Check the bytes:
`hexdump -Cv data/ticks.raw  | head -n 30`

Compare result file with expected:
`diff -s --strip-trailing-cr <(head -n 2000 data/rad_result.csv) data/ticks_result_sample.csv`

# Logs & run time (best of 5)

Debug:

```
184612 microseconds
1.15074 microseconds per tick
```

Release:

```
13965.938000 microseconds
0.087054 microseconds per tick
```

# Todo

- Extract Orderbook to a new file
- Consider sorting orders based on order id
