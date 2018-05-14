#!/bin/bash
      listen=10.99.0.100
      tmp=$(redis-cli -h $listen flushall)

 exit 0
