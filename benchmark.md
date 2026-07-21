## Benchmark of the single threaded epoll server using wrk

Command:
```
wrk -t4 -c1000 -d10s http://127.0.0.1:8080/
```

Result: (after commenting all `cout` statements)
```
Running 10s test @ http://127.0.0.1:8080/
  4 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    58.12ms  186.68ms   1.12s    92.86%
    Req/Sec    31.82k     3.61k   38.48k    89.36%
  1285778 requests in 11.21s, 125.07MB read
Requests/sec: 114719.72
Transfer/sec:     11.16MB
```