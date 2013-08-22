raspberrypi-incubator-utils
===========================

Utilities for working with
(https://github.com/Cat-Ion/raspberrypi-incubator)[raspberrypi-incubator].

At the moment this only includes a set of programs to delta-encode the
output and pack it (so differences smaller than ±3 only use up 4
bits). The resulting output compresses somewhat better than the
original plaintext. This isn't very useful, but was fun to write.

