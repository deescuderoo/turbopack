#define QUOTE(x) #x

#define START_TIMER(name) \
  auto start##name = std::chrono::high_resolution_clock::now()

#define STOP_TIMER(name)                                                       \
  auto stop##name = std::chrono::high_resolution_clock::now();                 \
  auto duration##name = std::chrono::duration_cast<std::chrono::microseconds>( \
      stop##name - start##name);                                               \
  std::cout << QUOTE(name) << ": " << duration##name.count() << " us\n"
