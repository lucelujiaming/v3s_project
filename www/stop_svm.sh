#!/bin/sh

# Stop svm and watch dog
# ps | grep svm | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" | sh 
# Stop svm and keep watch dog
ps | grep "/tools/svm" | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" | sh


