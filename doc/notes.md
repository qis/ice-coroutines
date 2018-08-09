# NOTES
- Implement timer and recv/send operations with a timeout. Possible implementation using cppcoro:

```cpp
task<size_t> read_with_timeout(io_service& io, socket& s, void* buf, size_t size)
{
  cancellation_source cs;
  auto [result, dummy] = co_await when_all(
    [&]() -> task<size_t> {
      auto cancelOnExit = on_scope_exit([&] { cs.request_cancellation(); });
      co_return co_await s.recv(buf, size, cs.token());
    }(),
    [&]() -> task<void> {
      auto cancelOnExit = on_scope_exit([&] { cs.request_cancellation(); });
      try { co_await io.schedule_after(100ms, cs.token()); } catch (operation_cancelled) {}
    }());
  co_return result;
}
```

Comment from Lewis:

```
It's unfortunately, probably a few extra heap allocations than is ideal...

And it also requires you to wait for the timer thread to respond to the cancellation of the timer once the operation
completes successfully which could add a little bit of latency.

But if instead of a per-read timeout you just need an overall timeout then you can probably reuse the same
cancellation_token for multiple calls to recv().
```
