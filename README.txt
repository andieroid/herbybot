# Robot Programming

## SUMMARY

The implemented simulation attempts to count grapes using a grape-tagging approach, whereby attributes of grapes appearing on the left of the image are stored so they can be tracked to the right - whereby all grapes appearing in the frame are counted and added to a tally; the process is repeated until the end waypoint is reached.  A second 'fallback' system is included (commented out) that attempts to count grapes based on time segments, given the known speed of the robot, each frame for counting can be estimated over each journey up and down the vine row.

### How to run this system
This repository contains a ROS-based simulated grape-counting robot system that traverses each side of a grape vine to count bunches of grapes.
