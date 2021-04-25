#ifndef Z21TYPES_H
#define Z21TYPES_H

enum Direction { Backward, Forward };

enum FromToZ21 { fromZ21, toZ21 };

enum ProgMode { Off, On };

enum class BoolState { Off, On, Undefined };

#define UNDEFINED_CV_VALUE -1
enum ProgResult { Success, Failure };

#endif