
#include <time.h>
#include "stopwatch.h"

double inline now() {
  return (float)clock()/CLOCKS_PER_SEC;
}

stopwatch::stopwatch() { startTime = cumTime = 0.0; }

void stopwatch::reset() { startTime = cumTime = 0.0; }

void stopwatch::start() {
  if (startTime == 0.0) startTime = now();
}

double stopwatch::stop() {
  if (startTime != 0.0) {
    cumTime += now() - startTime;
    startTime = 0.0;
  }
  return cumTime;
}

double stopwatch::split() {
  if (startTime != 0.0) {
    double t0 = now();
    cumTime += t0 - startTime;
    startTime = t0;
  }
  return cumTime;
}
