# 18742 Course Project: Exploring Lossy Cache Compression in On-Chip CPU Caches

This class project is the work of Jeffrey Luo, Shivani Prasad, and Deanyone Su.

We primarily plan to explore lossy cache compression to increase effective cache
capacity to overall lower AMAT for a simple CPU model. We hope to build a
reasonable architecture model for future work and a general proof-of-concept for
the use of lossy cache compression using fixed-function compression logic in
silicon.

## Tools

The primary work is intended to be based on pin, a binary instrumentation tool.
We instrument reads and writes from sample binaries to feed into our in-house
cache model to measure general performance.
