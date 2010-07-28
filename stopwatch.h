
class stopwatch {
 public:

  stopwatch();

  double startTime, cumTime;
  void start();
  double split();
  double stop();
  void reset();
};
